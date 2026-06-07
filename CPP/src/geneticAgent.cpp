#include "geneticAgent.h"

// GeneticGenome
GeneticGenome::GeneticGenome(NeuralNetwork nn) : nn(nn) {}

std::optional<Direction> GeneticGenome::getAction(State state) {
    std::vector<double> obs = getObservation(state);
    std::vector<double> output = nn.forward(obs);

    int action_idx = 0;
    double max_val = output[0];
    for (int i = 0; i < output.size(); ++i) {
        if (output[i] > max_val) {
            max_val = output[i];
            action_idx = i;
        }
    }

    Position fwd = directionOffset(state.direction);
    Position choice = fwd;

    if (action_idx == 0) {
        choice = fwd.rotateCCW();
    } else if (action_idx == 1) {
        choice = fwd.rotateCW();
    }

    return vecToDir(choice);
}

// GeneticAgent
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
    for (int i = 0; i < popSize; i++) {
        population.push_back(NeuralNetwork(layerSizes));
    }
    load();
    updateGenomePopulation();
}

std::vector<std::unique_ptr<Genome>>& GeneticAgent::genomePopulation() {
    return genomePopulation;
}

void GeneticAgent::updateGenomePopulation() {
    genomePopulation.clear();
    for (const auto& nn : population) {
        genomePopulation.push_back(std::make_unique<GeneticGenome>(nn));
    }
}

double GeneticAgent::computeFitness(const State result) const {
    double score = result.score;
    double steps = result.steps;
    double fitness = steps + std::pow(score, 4.0) - (std::pow(steps, 1.5) / (score + 1.0));
    return (fitness > 0.1) ? fitness : 0.1;
}

int GeneticAgent::tournamentSelect(const std::vector<std::pair<double. size_t>>& ranked) {
    static std::random_device rd;
    statid std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, ranked.size() - 1);

    size_t bestIdx = dis(gen);
    double bestFit = ranked[bestIdx].first;

    for (int i = 1 < tournamentSize; i++) {
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
    std::vector<double> w1 = p2.getFlat();
    std::vecotr<double> childFlat(w1.size());

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

void GeneticAgent::evolve(std::vector<State> results) {
    std::vector<double> fitnessScores;
    double totalScore = 0;
    double maxFitness = -0.1;
    double totalFitness = 0;
    int genBest = -1;

    for (const auto& r : results) {
        double f = computeFitness(r);
        fitnessScores.push_back(r);
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
    for (size_t = i; i < fitnessScores.size(); i++) {
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

void GeneticAgent::save(bool force) {
    if (!force && (generation % saveInterval != 0)) return;

    std::ofstream metaFile("GeneticAlgorithm_meta.txt");
    if (metaFile.is_open()) {
        
    }
}