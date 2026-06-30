// trainer.h

#pragma once

#include <chrono>
#include <optional>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <future>

#include "agent.h"
#include "types.h"
#include "env.h"

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

struct GenerationStats {
    int generation = 0;

    std::vector<int> scores;
    std::vector<float> totalRewards;
    std::vector<int> steps;

    double durationSeconds = 0.0;

    double avgScore()  const;
    int    maxScore()  const;
    double avgReward() const;
    double avgSteps()  const;

    std::string summary() const;
};

struct TrainerConfig {
    std::optional<int> maxGenerations;
    std::optional<double> timeLimitMinutes;

    int envWidth = 20;
    int envHeight = 20;

    int nWorkers = 0;

    bool verbose = false;

    int resolvedWorkers() const {
        if (nWorkers > 0) return nWorkers;
        unsigned int hw = std::thread::hardware_concurrency();
        return static_cast<int>(hw > 0 ? hw : 1);
    }
};

// ---------------------------------------------------------------------------
// Trainer
// ---------------------------------------------------------------------------

class Trainer {
public:
    Trainer(Agent& agent, TrainerConfig config);

    std::vector<GenerationStats> run();
    void stop();

    int currentGeneration() const { return generation; }
    const std::vector<GenerationStats>& history() const { return history_; }

private:
    Agent& agent;
    TrainerConfig config;

    int generation = 0;
    bool stopRequested = false;
    std::vector<GenerationStats> history_;

    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    bool shouldStop(TimePoint runStart) const;

    GenerationStats runGeneration(int genNum, TimePoint runStart);
    GenerationStats runPopulationGeneration(PopulationAgent& agent, int genNum, TimePoint runStart);
    GenerationStats runSingleGeneration(Agent& agent, int genNum, TimePoint runStart);

    std::vector<State> evaluatePopulation(
        std::vector<std::unique_ptr<Genome>>& population);
    std::vector<State> evaluatePopulationParallel(
        std::vector<std::unique_ptr<Genome>>& population);
    std::vector<State> evaluatePopulationSequential(
        std::vector<std::unique_ptr<Genome>>& population);

    State runEpisode(Genome& genome);
    State runEpisode(Agent& agent);
};