#include "geneticAgent.h"
#include <random>

// ---------------------------------------------------------------------------------
// Parameter helpers
// ---------------------------------------------------------------------------------

static std::map<std::string, std::string> defaultParams() {
    return {
        {"pop_size",              "200"},
        {"elite_count",           "5"},
        {"mutation_rate",         "0.10"},
        {"mutation_strength",     "0.20"},
        {"min_mutation_strength", "0.05"},
        {"mut_stren_dropoff",     "40"},
        {"tournament_size",       "5"},
        {"save_interval",         "20"},
        {"hidden_layers",         "[16]"},
        {"input_components",      "relative_apple,direction,snake_size,distance_to_walls,distance_to_danger"}
    };
}

static std::vector<std::string> splitComponents(const std::string& csv) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : csv) {
        if (c == ',') { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else if (!std::isspace((unsigned char)c)) cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// ---------------------------------------------------------------------------------
// Genetic Genome
// ---------------------------------------------------------------------------------

GeneticGenome::GeneticGenome(NeuralNetwork nn, std::vector<std::string> components)
    : nn(nn), components(std::move(components)) {}

std::optional<Direction> GeneticGenome::getAction(State state) {
    const auto obs = getObservation(state, components);
    const std::vector<double> output = nn.forward(obs);

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

GeneticAgent::GeneticAgent(std::string agentName, std::map<std::string, std::string> paramsIn)
    : generation(0),
      bestScoreEver(0),
      gen(std::random_device{}()),
      agentName(std::move(agentName)),
      params(defaultParams()),
      registry(this->agentName),
      configId(-1) {
    for (auto& [k, v] : paramsIn) params[k] = v;

    popSize             = std::stoi(params.at("pop_size"));
    eliteCount          = std::stoi(params.at("elite_count"));
    mutationRate        = std::stod(params.at("mutation_rate"));
    mutationStrength    = std::stod(params.at("mutation_strength"));
    minMutationStrength = std::stod(params.at("min_mutation_strength"));
    mutStrenDropoff     = std::stoi(params.at("mut_stren_dropoff"));
    tournamentSize      = std::stoi(params.at("tournament_size"));
    saveInterval        = std::stoi(params.at("save_interval"));

    components = splitComponents(params.at("input_components"));

    std::vector<int> hidden = jsonParseIntArray(params.at("hidden_layers"));
    if (hidden.empty()) hidden = {16};
    layerSizes.clear();
    layerSizes.push_back(observationSize(components));
    for (int h : hidden) layerSizes.push_back(h);
    layerSizes.push_back(3);

    load();
    if (population.empty()) {
        population.reserve(popSize);
        for (int i = 0; i < popSize; i++) {
            NeuralNetwork nn(layerSizes);
            nn.randomize(this->gen);
            population.push_back(nn);
        }
    }

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
    long   totalSteps   = 0;
    int genBest         = -1;

    for (const auto& r : results) {
        double f = computeFitness(r);
        fitnessScores.push_back(f);
        if (r.score > genBest) genBest = r.score;
        if (f > maxFitness) maxFitness = f;
        totalScore += r.score;
        totalFitness += f;
        totalSteps += r.steps;
    }

    const double avgScore   = totalScore / results.size();
    const double avgSteps   = static_cast<double>(totalSteps) / results.size();
    const double avgFitness = totalFitness / results.size();

    if (avgScore > bestAvgScore) {
        bestAvgScore        = avgScore;
        bestAvgScoreSteps   = avgSteps;
        bestAvgScoreFitness = avgFitness;
    }

    const bool isNewBest = genBest > bestScoreEver;
    if (isNewBest) bestScoreEver = genBest;

    std::cout << "[GeneticAgent] Gen" << generation
              << " | Best Score: " << genBest
              << " | Avg Score: " << avgScore
              << " | Avg Steps: " << avgSteps
              << " | Best Fitness: " << maxFitness
              << " | Avg Fitness: " << avgFitness
              << std::endl;

    std::vector<std::pair<double, size_t>> ranked;
    ranked.reserve(fitnessScores.size());
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
        const int p1_idx = tournamentSelect(ranked);
        const int p2_idx = tournamentSelect(ranked);

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
    return GeneticGenome(population[0], components).getAction(state);
}

void GeneticAgent::atTrainingEnd() {
    std::cout << "[GeneticAgent]\n"
              << "  final generation: " << generation << "\n"
              << "  max score: " << bestScoreEver << "\n"
              << "  max average score: " << bestAvgScore << "\n"
              << "  average steps: " << bestAvgScoreSteps << "\n"
              << "  average fitness: " << bestAvgScoreFitness << "\n";
}

// ---------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------

void GeneticAgent::updateGenomePopulation() {
    genomePopulation.clear();
    genomePopulation.reserve(population.size());
    for (const auto& nn : population) {
        genomePopulation.push_back(std::make_unique<GeneticGenome>(nn, components));
    }
}

double GeneticAgent::computeFitness(const State& result) const {
    const double score = result.score;
    const double steps = result.steps;
    const double avg_steps = steps / (score > 0 ? score : 1.0);
    const double fitness = score > 0 ? (score + 0.1) * (1000.0 - avg_steps) : avg_steps;
    return (fitness > 0.1) ? fitness : 0.1;
}

int GeneticAgent::tournamentSelect(const std::vector<std::pair<double, size_t>>& ranked) {
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

    std::uniform_real_distribution<double> dis(0.0, 1.0);

    for (size_t i = 0; i < w1.size(); i++) {
        childFlat[i] = (dis(gen) < 0.5) ? w1[i] : w2[i];
    }
    return childFlat;
}

std::vector<double> GeneticAgent::mutate(std::vector<double> weights) {
    std::uniform_real_distribution<double> uniform_dis(0.0, 1.0);
    std::normal_distribution<double> normal_dis(0.0, 1.0);

    double mutStren = mutationStrength - 0.01 * (static_cast<double>(generation) / mutStrenDropoff);
    if (mutStren < minMutationStrength) mutStren = minMutationStrength;

    for (auto& w : weights) {
        if (uniform_dis(gen) < mutationRate) {
            w += normal_dis(gen) * mutStren;
        }
    }
    return weights;
}

// ---------------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------------

void GeneticAgent::save(bool force) {
    if (!force && (generation % saveInterval != 0)) return;

    if (configId == -1) configId = registry.createNew(params).id;

    std::filesystem::create_directories("../checkpoints");
    const std::string weightsPath = registry.weightsPath(configId);

    std::ofstream wf(weightsPath, std::ios::binary);
    if (!wf.is_open()) {
        std::cerr << "[GeneticAgent] Could not open " << weightsPath << " for writing.\n";
        return;
    }
    for (const auto& nn : population) {
        const std::vector<double> flat = nn.getFlat();
        wf.write(reinterpret_cast<const char*>(flat.data()),
                static_cast<std::streamsize>(flat.size() * sizeof(double)));
    }
    wf.close();

    registry.update(configId, generation, bestScoreEver);

    std::cout << "[GeneticAgent] Checkpoint saved (config #" << configId
              << ", gen " << generation << ").\n";
}

void GeneticAgent::load() {
    auto rec = registry.findByParams(params);
    if (!rec) {
        std::cout << "[GeneticAgent] New configuration; starting fresh.\n";
        return;
    }

    configId = rec->id;
    generation = rec->generation;
    bestScoreEver = rec->bestScore;

    std::ifstream wf(registry.weightsPath(configId), std::ios::binary);
    if (!wf.is_open()) {
        std::cout << "[GeneticAgent] Config #" << configId
                  << " has no weights file; starting with random weights.\n";
        return;
    }

    const size_t flatSize = NeuralNetwork(layerSizes).getFlat().size();
    std::vector<NeuralNetwork> loaded;
    loaded.reserve(popSize);
    std::vector<double> flat(flatSize);
    for (int i = 0; i < popSize; i++) {
        wf.read(reinterpret_cast<char*>(flat.data()),
                static_cast<std::streamsize>(flatSize * sizeof(double)));
        if (wf.gcount() != static_cast<std::streamsize>(flatSize * sizeof(double))) break;
        NeuralNetwork nn(layerSizes);
        nn.setFlat(flat);
        loaded.push_back(std::move(nn));
    }
    wf.close();

    if (loaded.empty()) return;
    population = std::move(loaded);

    std::cout << "[GeneticAgent] Loaded config #" << configId << ": gen " << generation
              << ", best score " << bestScoreEver << std::endl;
}