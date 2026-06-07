// geneticAgent.h

#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "agent.h"
#include "neuralNetwork.h"
#include "types.h"

class GeneticGenome : public Genome {
public:
    GeneticGenome(NeuralNetwork nn);
    std::optional<Direction> getAction(State state) override;
    const NeuralNetwork& getNN() const { return nn; }

private:
    NeuralNetwork nn;
};

class GeneticAgent : public PopulationAgent, public Agent {
public:
    GeneticAgent(
        int popSize = 200,
        int eliteCount = 5,
        double mutationRate = 0.10,
        double mutationStrength = 0.2,
        double minMutationStrength = 0.05,
        int mutStrenDropoff = 40,
        int tournamentSize = 5,
        int saveInterval = 20
    );

    std::vector<std::unique_ptr<Genome>>& getPopulation() override;
    void evolve(std::vector<State> results) override;
    std::optional<Direction> getAction(State state) override;

private:
    int popSize;
    int eliteCount;
    double mutationRate;
    double mutationStrength;
    double minMutationStrength;
    int mutStrenDropoff;
    int tournamentSize;
    int saveInterval;

    std::vector<int> layerSizes;
    int generation;
    int bestScoreEver;

    std::vector<NeuralNetwork> population;
    std::vector<std::unique_ptr<Genome>> genomePopulation;

    // Evolution
    double computeFitness(const State& results) const;
    int tournamentSelect(const std::vector<std::pair<double, size_t>>& ranked);
    std::vector<double> crossover(const NeuralNetwork& p1, const NeuralNetwork& p2);
    std::vector<double> mutate(std::vector<double> weights);

    void updateGenomePopulation();
    void save(bool force = false);
    void load();
};