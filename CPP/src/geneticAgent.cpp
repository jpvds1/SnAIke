#include "../include/geneticAgent.h"

static constexpr const char* CKPT_DIR      = "./checkpoints";
static constexpr const char* META_FILE     = "GeneticAlgorithm_meta.json";
static constexpr const char* WEIGHTS_FILE  = "GeneticAlgorithm_weights.json";
static constexpr double      SCHEMA_VERSION = 3.0;

// ---------------------------------------------------------------------------------
// JSON Helpers
// ---------------------------------------------------------------------------------

static std::string jsonGetRaw(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
    if (pos >= json.size()) return "";

    char open = json[pos];
    if (open == '[' || open == '{') {
        char close = (open == '[') ? ']' : '}';
        int depth = 1;
        size_t start = pos;
        pos++;
        while (pos < json.size() && depth > 0) {
            if (json[pos] == open) depth++;
            if (json[pos] == close) depth--;
            pos++;
        }
        return json.substr(start, pos - start);
    } else {
        size_t start = pos;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\n') pos++;
        std::string raw = json.substr(start, pos - start);

        while (!raw.empty() && std::isspace((unsigned char)raw.front())) raw.erase(raw.begin());
        while (!raw.empty() && std::isspace((unsigned char)raw.back())) raw.pop_back();
        if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
            raw = raw.substr(1, raw.size() - 2);
        }
        return raw;
    }
}

static double jsonGetDouble(const std::string& json, const std::string& key, double def = 0.0) {
    auto s = jsonGetRaw(json, key);
    if (s.empty()) return def;
    try {
        return std::stod(s);
    } catch (...) {
        return def;
    }
}

static int jsonGetInt(const std::string& json, const std::string& key, int def = 0) {
    auto s = jsonGetRaw(json, key);
    if (s.empty()) return def;
    try {
        return std::stoi(s);
    } catch (...) {
        return def;
    }
}

static std::string jsonGetStr(const std::string& json, const std::string& key, const std::string& def = "") {
    auto s = jsonGetRaw(json, key);
    return s.empty() ? def : s;
}

static std::vector<int> jsonParseIntArray(const std::string& arr) {
    std::vector<int> out;
    bool inside = false;
    std::string cur;
    for (char c : arr) {
        if (c == '[') { inside = true; continue; }
        if (c == ']') { if (!cur.empty()) { try { out.push_back(std::stoi(cur)); } catch (...) {} cur.clear(); } break; }
        if (inside && (std::isdigit((unsigned char)c) || c == '-')) cur += c;
        else if (inside && c == ',') { if (!cur.empty()) { try { out.push_back(std::stoi(cur)); } catch (...) {} cur.clear(); } }
    }
    return out;
}


// ---------------------------------------------------------------------------------
// Genetic Genome
// ---------------------------------------------------------------------------------

GeneticGenome::GeneticGenome(NeuralNetwork nn) : nn(nn) {}

std::optional<Direction> GeneticGenome::getAction(State state) {
    std::vector<double> obs = getObservation(state);
    std::vector<double> output = nn.forward(obs);

    int action_idx = 0;
    double max_val = output[0];
    for (int i = 1; i < output.size(); ++i) {
        if (output[i] > max_val) {
            max_val = output[i];
            action_idx = static_cast<int>(i);
        }
    }

    Position fwd = directionOffset(state.direction);
    Position choice = fwd;

    if      (action_idx == 0) choice = fwd.rotateCCW();
    else if (action_idx == 1) choice = fwd.rotateCW();

    return vecToDir(choice);
}

// ---------------------------------------------------------------------------------
// Gemetic Agent
// ---------------------------------------------------------------------------------

GeneticAgent::GeneticAgent(
    int popSize,
    int eliteCount,
    double mutationRate,
    double mutationStrength,
    double minMutationStrength,
    int mutStrenDropoff,
    int tournamentSize,
    int saveInterval
) : 
popSize(popSize),
eliteCount(eliteCount),
mutationRate(mutationRate),
mutationStrength(mutationStrength),
minMutationStrength(minMutationStrength),
mutStrenDropoff(mutStrenDropoff),
tournamentSize(tournamentSize),
saveInterval(saveInterval) {
    population.reserve(popSize);
    for (int i = 0; i < popSize; i++) {
        population.push_back(NeuralNetwork(layerSizes));
    }
    load();
    updateGenomePopulation();
}

// ---------------------------------------------------------------------------------
// Population Agent
// ---------------------------------------------------------------------------------

std::vector<std::unique_ptr<Genome>>& GeneticAgent::getPopulation() {
    return genomePopulation;
}

void GeneticAgent::evolve(std::vector<State> results) {
    std::vector<double> fitnessScores;
    fitnessScores.reserve(results.size());

    double totalScore   = 0.0;
    double maxFitness   = -0.1;
    double totalFitness = 0.0;
    int genBest         = -1;

    for (const auto& r : results) {
        double f = computeFitness(r);
        fitnessScores.push_back(f);
        if (r.score > genBest) genBest = r.score;
        if (f > maxFitness) maxFitness = f;
        totalScore += r.score;
        totalFitness += f;
    }

    bool isNewBest = genBest > bestScoreEver;
    if (isNewBest) bestScoreEver = genBest;

    std::cout << "[GeneticAgent] Gen" << generation
              << " | Best Score: " << genBest
              << " | Avg Score: " << (totalScore / results.size())
              << " | Best Fitness: " << maxFitness
              << " | Avg Fitness: " << (totalFitness / results.size())
              << std::endl;

    std::vector<std::pair<double, size_t>> ranked;
    for (size_t i = 0; i < fitnessScores.size(); i++) {
        ranked.push_back({fitnessScores[i], i});
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    std::vector<NeuralNetwork> newPopulation;
    newPopulation.reserve(popSize);

    for (int i = 0; i < eliteCount; i++) {
        newPopulation.push_back(population[ranked[i].second]);
    }

    while (newPopulation.size() < static_cast<size_t>(popSize)) {
        int p1_idx = tournamentSelect(ranked);
        int p2_idx = tournamentSelect(ranked);

        std::vector<double> childWeights = mutate(crossover(population[p1_idx], population[p2_idx]));
        NeuralNetwork child(layerSizes);
        child.setFlat(childWeights);
        newPopulation.push_back(child);
    }

    population = std::move(newPopulation);
    generation++;

    updateGenomePopulation();
    save(isNewBest);
}

// ---------------------------------------------------------------------------------
// Agent
// ---------------------------------------------------------------------------------

std::optional<Direction> GeneticAgent::getAction(State state) {
    return GeneticGenome(population[0]).getAction(state);
}

// ---------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------

void GeneticAgent::updateGenomePopulation() {
    genomePopulation.clear();
    genomePopulation.reserve(population.size());
    for (const auto& nn : population) {
        genomePopulation.push_back(std::make_unique<GeneticGenome>(nn));
    }
}

double GeneticAgent::computeFitness(const State& result) const {
    double score = result.score;
    double steps = result.steps;
    double fitness = steps + std::pow(score, 4.0) - (std::pow(steps, 1.5) / (score + 1.0));
    return (fitness > 0.1) ? fitness : 0.1;
}

int GeneticAgent::tournamentSelect(const std::vector<std::pair<double, size_t>>& ranked) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, ranked.size() - 1);

    size_t bestIdx = dis(gen);
    double bestFit = ranked[bestIdx].first;

    for (int i = 1; i < tournamentSize; i++) {
        size_t idx = dis(gen);
        if (ranked[idx].first > bestFit) {
            bestFit = ranked[idx].first;
            bestIdx = idx;
        }
    }
    return ranked[bestIdx].second;
}

std::vector<double> GeneticAgent::crossover(const NeuralNetwork& p1, const NeuralNetwork& p2) {
    std::vector<double> w1 = p1.getFlat();
    std::vector<double> w2 = p2.getFlat();
    std::vector<double> childFlat(w1.size());

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    for (size_t i = 0; i < w1.size(); i++) {
        childFlat[i] = (dis(gen) < 0.5) ? w1[i] : w2[i];
    }
    return childFlat;
}

std::vector<double> GeneticAgent::mutate(std::vector<double> weights) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> uniform_dis(0.0, 1.0);
    std::normal_distribution<double> normal_dis(0.0, 1.0);

    double mutStren = mutationStrength - 0.01 * (generation / mutStrenDropoff);
    if (mutStren < minMutationStrength) mutStren = minMutationStrength;

    for (size_t i = 0; i < weights.size(); i++) {
        if (uniform_dis(gen) < mutationRate) {
            weights[i] += normal_dis(gen) * mutStren;
        }
    }
    return weights;
}

// ---------------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------------

void GeneticAgent::save(bool force) {
    if (!force && (generation % saveInterval != 0)) return;

    std::filesystem::create_directories(CKPT_DIR);

    std::string metaPath = std::string(CKPT_DIR) + "/" + META_FILE;
    std::ofstream meta(metaPath);
    if (!meta.is_open()) {
        std::cerr << "[GeneticAgent] Could not open " << metaPath << " for writing." << std::endl;
        return;
    }

    std::ostringstream lsArr;
    lsArr << "[";
    for (size_t i = 0; i < layerSizes.size(); i++) {
        if (i) lsArr << ", ";
        lsArr << layerSizes[i];
    }
    lsArr << "]";

    meta << "{\n"
         << "  \"agent\": \"GeneticAlgorithm\",\n"
         << "  \"version\": " << SCHEMA_VERSION << ",\n"
         << "  \"generation\": " << generation << ",\n"
         << "  \"best_score_ever\": " << bestScoreEver << ",\n"
         << "  \"hyperparameters\": {\n"
         << "    \"pop_size\": "               << popSize             << ",\n"
         << "    \"elite_count\": "            << eliteCount          << ",\n"
         << "    \"mutation_rate\": "          << mutationRate        << ",\n"
         << "    \"mutation_strength\": "      << mutationStrength    << ",\n"
         << "    \"min_mutation_strength\": "  << minMutationStrength << ",\n"
         << "    \"mut_stren_dropoff\": "      << mutStrenDropoff     << ",\n"
         << "    \"tournament_size\": "        << tournamentSize      << ",\n"
         << "    \"layer_sizes\": "            << lsArr.str()         << ",\n"
         << "    \"save_interval\": "          << saveInterval        << "\n"
         << "  }\n"
         << "}\n";
    meta.close();

    std::string weightsPath = std::string(CKPT_DIR) + "/" + WEIGHTS_FILE;
    std::ofstream wf(weightsPath, std::ios::binary);
    if (!wf.is_open()) {
        std::cerr << "[GeneticAgent] Could not open " << weightsPath << " for writing.\n";
        return;
    }

    for (const auto& nn : population) {
        std::vector<double> flat = nn.getFlat();
        wf.write(reinterpret_cast<const char*>(flat.data()),
                static_cast<std::streamsize>(flat.size() * sizeof(double)));
    }
    wf.close();

    std::cout << "[GeneticAgent] Checkpoint saved (gen " << generation << ").\n";
}

void GeneticAgent::load() {
    std::string metaPath = std::string(CKPT_DIR) + "/" + META_FILE;
    std::string weightsPath = std::string(CKPT_DIR) + "/" + WEIGHTS_FILE;

    std::ifstream meta(metaPath);
    if (!meta.is_open()) {
        std::cout << "[GeneticAlgorithm] No available save file found.\n";
        return;
    }

    std::string json((std::istreambuf_iterator<char>(meta)),
                      std::istreambuf_iterator<char>());
    meta.close();

    if (jsonGetStr(json, "agent") != "GeneticAgent")
        throw std::runtime_error("[GeneticAgent] Agent mismatch in checkpoint metadata.");

    double ver = jsonGetDouble(json, "version", -0.1);
    if (ver != SCHEMA_VERSION) {
        std::ostringstream err;
        err << "[GeneticAgent] Schema version mismatch: expected "
            << SCHEMA_VERSION << " got " << ver;
        throw std::runtime_error(err.str());
    }

    generation = jsonGetInt(json, "generation", generation);
    bestScoreEver = jsonGetInt(json, "best_score_ever", bestScoreEver);

    std::string hpRaw = jsonGetRaw(json, "hyperparameters");
    if (!hpRaw.empty()) {
        popSize             = jsonGetInt(hpRaw, "pop_size", popSize);
        eliteCount          = jsonGetInt(hpRaw, "elite_count", eliteCount);
        mutationRate        = jsonGetDouble(hpRaw, "mutation_rate", mutationRate);
        mutationStrength    = jsonGetDouble(hpRaw, "mutation_strength", mutationStrength);
        minMutationStrength = jsonGetDouble(hpRaw, "min_mutation_strength", minMutationStrength);
        mutStrenDropoff     = jsonGetInt(hpRaw, "min_stren_dropoff", mutStrenDropoff);
        tournamentSize      = jsonGetInt(hpRaw, "tournament_size", tournamentSize);
        saveInterval        = jsonGetInt(hpRaw, "save_intervael", saveInterval);

        std::string lsRaw = jsonGetRaw(hpRaw, "layer_sizes");
        auto ls = jsonParseIntArray(lsRaw);
        if (!ls.empty())
            const_cast<std::vector<int>&>(layerSizes) = ls;
    }

    while (static_cast<int>(population.size()) < popSize)
        population.emplace_back(layerSizes);
    population.resize(popSize, NeuralNetwork(layerSizes));

    std::ifstream wf(weightsPath, std::ios::binary);
    if (!wf.is_open()) {
        std::cout << "[GeneticAgent] No weights file found; starting with random weights.\n";
        return;
    }

    NeuralNetwork probe(layerSizes);
    size_t flatSize = probe.getFlat().size();

    std::vector<double> flat(flatSize);
    for (int i = 0; i < popSize; i++) {
        wf.read(reinterpret_cast<char*>(flat.data()),
                static_cast<std::streamsize>(flatSize * sizeof(double)));
        if (wf.gcount() != static_cast<std::streamsize>(flatSize * sizeof(double))) break;
        population[i].setFlat(flat);
    }
    wf.close();

    std::cout << "[GeneticAgent] Loaded checkpoint: gen " << generation
              << ", best score " << bestScoreEver << std::endl;
}