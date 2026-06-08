#include "../include/trainer.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// GenerationStats
// ---------------------------------------------------------------------------

double GenerationStats::avgScore() const {
    if (scores.empty()) return 0.0;
    return static_cast<double>(std::accumulate(scores.begin(), scores.end(), 0))
            / scores.size();
}

int GenerationStats::maxScore() const {
    if (scores.empty()) return 0;
    return *std::max_element(scores.begin(), scores.end());
}

double GenerationStats::avgReward() const {
    if (totalRewards.empty()) return 0.0;
    return static_cast<double>(
        std::accumulate(totalRewards.begin(), totalRewards.end(), 0.0f))
        / totalRewards.size();
}

double GenerationStats::avgSteps() const {
    if (steps.empty()) return 0.0;
    return static_cast<double>(std::accumulate(steps.begin(), steps.end(), 0))
            / steps.size();
}

std::string GenerationStats::summary() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Gen " << std::setw(4) << generation
       << " | avg_score=" << avgScore()
       << " max=" << maxScore()
       << "  avg_reward=" << std::setprecision(3) << avgReward()
       << "  episodes=" << scores.size()
       << "  elapsed" << std::setprecision(1) << durationSeconds << "s";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Trainer public
// ---------------------------------------------------------------------------

Trainer::Trainer(Agent& agent, TrainerConfig config) 
    : agent(agent), config(std::move(config)) {

    if (!config.maxGenerations && !config.timeLimitMinutes) {
        throw std::invalid_argument(
            "[Trainer] Trainer config must set at least one of "
            "maxGenerations or timeLimitMinutes"
        );
    }
}

std::vector<GenerationStats> Trainer::run() {
    stopRequested = false;
    history_.clear();
    generation = 0;

    const TimePoint runStart = Clock::now();

    while (!shouldStop(runStart)) {
        GenerationStats stats = runGeneration(generation, runStart);
        history_.push_back(stats);
        std::cout << stats.summary() << "\n"; 
        generation++;
    }

    std::cout << "[Trainer] Training complete. "
              << generation << " generation(s) run.\n";

    return history_;
}

void Trainer::stop() {
    stopRequested = true;
}

// ---------------------------------------------------------------------------
// Trainer private
// ---------------------------------------------------------------------------

bool Trainer::shouldStop(TimePoint runStart) const {
    if (stopRequested) return true;

    if (config.maxGenerations && generation >= * config.maxGenerations)
        return true;

    if (config.timeLimitMinutes) {
        const double elapsed = std::chrono::duration<double>(Clock::now() - runStart).count() / 60.0;
        if (elapsed >= *config.timeLimitMinutes) return true;
    }

    return false;
}

GenerationStats Trainer::runGeneration(int genNum, TimePoint runStart) {
    if (auto* pop = dynamic_cast<PopulationAgent*>(&agent)) {
        return runPopulationGeneration(*pop, genNum, runStart);
    }
    return runSingleGeneration(agent, genNum, runStart);
}

GenerationStats Trainer::runPopulationGeneration(PopulationAgent& agent, int genNum, TimePoint runStart) {
    const auto genStart = Clock::now();

    auto& population = agent.getPopulation();
    std::vector<State> results = evaluatePopulation(population);

    agent.train(results);

    GenerationStats stats;
    stats.generation = genNum;
    stats.durationSeconds =
        std::chrono::duration<double>(Clock::now() - genStart).count();

    stats.scores.reserve(results.size());
    stats.steps.reserve(results.size());

    for (const auto& s : results) {
        stats.scores.push_back(s.score);
        stats.steps.push_back(s.steps);
    }

    return stats;
}

GenerationStats Trainer::runSingleGeneration(Agent& agent, int genNum, TimePoint runStart) {
    const auto genStart = Clock::now();

    State result = runEpisode(agent);
    agent.train({ result });

    GenerationStats stats;
    stats.generation = genNum;
    stats.durationSeconds =
        std::chrono::duration<double>(Clock::now() - genStart).count();
    stats.scores.push_back(result.score);
    stats.steps.push_back(result.steps);

    return stats;
}

std::vector<State> Trainer::evaluatePopulation(std::vector<std::unique_ptr<Genome>>& population) {
    std::vector<State> results;
    results.reserve(population.size());

    for (auto& genome : population) {
        results.push_back(runEpisode(*genome));
    }

    return results;
}

State Trainer::runEpisode(Genome& genome) {
    SnakeEnv env;
    State obs = env.reset();
    bool done = false;

    while (!done) {
        auto action = genome.getAction(obs);
        GymState gs = env.step(action);
        obs = gs.state;
        done = gs.done;
    }

    return env.getLastState();
}

State Trainer::runEpisode(Agent& agent) {
    SnakeEnv env;
    State obs = env.reset();
    bool done = false;

    while (!done) {
        auto action = agent.getAction(obs);
        GymState gs = env.step(action);
        obs = gs.state;
        done = gs.done;
    }

    return env.getLastState();
}