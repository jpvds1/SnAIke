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
#include <cctype>
#include <cstddef>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>


#include <map>

#include "helpers.h"
#include "json.h"
#include "agent.h"
#include "neuralNetwork.h"
#include "types.h"
#include "configRegistry.h"

class GeneticGenome : public Genome {
public:
    GeneticGenome(NeuralNetwork nn, std::vector<std::string> components);
    std::optional<Direction> getAction(State state) override;
    const NeuralNetwork& getNN() const { return nn; }

private:
    NeuralNetwork nn;
    std::vector<std::string> components;
};

class GeneticAgent : public PopulationAgent {
public:
    GeneticAgent(std::string agentName, std::map<std::string, std::string> params);

    std::vector<std::unique_ptr<Genome>>& getPopulation() override;
    void evolve(std::vector<State> results) override;
    std::optional<Direction> getAction(State state) override;
    int atTrainingStart() override;
    void atTrainingEnd() override;
    AgentSnapshot takeSnapshot() override;

private:
    int popSize;
    int eliteCount;
    double mutationRate;
    double mutationStrength;
    double minMutationStrength;
    int mutStrenDropoff;
    int tournamentSize;
    int saveInterval;

    std::vector<std::string> components;
    std::vector<int> layerSizes;
    std::string fitnessFunction;
    int generation;
    int bestScoreEver;

    double bestAvgScore = -1.0;
    double bestAvgScoreSteps = 0.0;
    double bestAvgScoreFitness = 0.0;

    double intervalMaxScore = 0.0;
    double intervalScoreSum = 0.0;
    double intervalStepsSum = 0.0;
    double intervalFitnessSum = 0.0;
    int    intervalCount = 0;

    std::vector<NeuralNetwork> population;
    std::vector<std::unique_ptr<Genome>> genomePopulation;

    std::mt19937 gen;

    // Persistence identity
    std::string agentName;
    std::map<std::string, std::string> params;
    ConfigRegistry registry;
    int configId;

    // Evolution
    double computeFitness(const State& results) const;
    double computeFitnessScoreSteps(const State& result) const;
    double computeFitnessSurvival(const State& result) const;
    double computeFitnessEfficiency(const State& result) const;
    int tournamentSelect(const std::vector<std::pair<double, size_t>>& ranked);
    std::vector<double> crossover(const NeuralNetwork& p1, const NeuralNetwork& p2);
    std::vector<double> mutate(std::vector<double> weights);

    void updateGenomePopulation();
    void save(bool force = false);
    void load();
};