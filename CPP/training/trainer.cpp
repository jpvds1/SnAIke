#include "trainer.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <future>
#include <iomanip>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <utility>
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
// Milestone table
// ---------------------------------------------------------------------------

static bool isMilestone(int gen) {
    if (gen <= 0)     return false;
    if (gen <= 5000)  return gen % 1000 == 0;
    if (gen <= 10000) return gen % 2500 == 0;
    return gen % 5000 == 0;
}

static void printTableHeader() {
    std::cout << "| Generation | Max Score | Average Score | Average Steps | Average Fitness | Elapsed Time | Elapsed Time / Generation |\n"
              << "| ---------- | --------- | ------------- | ------------- | --------------- | ------------ | ------------------------- |\n";
}

static void printMilestoneRow(int gen, const AgentSnapshot& s, double dt, int dGen) {
    std::ostringstream row;
    row << std::fixed
        << "| " << gen
        << " | " << std::setprecision(0) << s.maxScore
        << " | " << std::setprecision(2) << s.avgScore
        << " | " << s.avgSteps
        << " | " << s.avgFitness
        << " | " << std::setprecision(1) << dt << "s"
        << " | " << std::setprecision(4) << (dGen > 0 ? dt / dGen : 0.0) << "s |";
    std::cout << row.str() << std::endl;
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
    agent.setVerbose(config.verbose);
    generation = agent.atTrainingStart();
    const int start_generation = generation;

    const int workers = config.resolvedWorkers();
    std::cout << "[Trainer] Starting - workers=" << workers;
    if (config.maxGenerations) std::cout << "  max_gen=" << *config.maxGenerations;
    if (config.timeLimitMinutes) std::cout << "  time_limit=" << *config.timeLimitMinutes << "min";
    std::cout << "\n";

    printTableHeader();

    const TimePoint runStart = Clock::now();
    TimePoint lastMilestone = runStart;
    int lastMilestoneGen = generation;

    while (!shouldStop(runStart)) {
        GenerationStats stats = runGeneration(generation, runStart);
        history_.push_back(stats);
        if (config.verbose) std::cout << stats.summary() << "\n";
        generation++;

        if (isMilestone(generation)) {
            const double dt = std::chrono::duration<double>(Clock::now() - lastMilestone).count();
            printMilestoneRow(generation, agent.takeSnapshot(), dt, generation - lastMilestoneGen);
            lastMilestone = Clock::now();
            lastMilestoneGen = generation;
        }
    }

    const double totalElapsed = std::chrono::duration<double>(Clock::now() - runStart).count();
    std::cout << "[Trainer] Training complete.\n"
              << "  generations trained: " << generation - start_generation << "\n"
              << "  elapsed time: " << std::fixed << std::setprecision(1) << totalElapsed << "s\n"
              << "  elapsed time / generation: " << std::setprecision(4)
              << (generation > 0 ? totalElapsed / generation : 0.0) << "s\n";

    agent.atTrainingEnd();

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
    std::vector<State> results = evaluatePopulationParallel(population);

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

std::vector<State> Trainer::evaluatePopulationParallel(std::vector<std::unique_ptr<Genome>>& population) {
    const int nWorkers = config.resolvedWorkers();
    const size_t popSize = population.size();
    const size_t batchSize = static_cast<size_t>(nWorkers);

    std::vector<State> results(popSize);
    bool fellBack = false;

    for (size_t batchStart = 0; batchStart < popSize; batchStart += batchSize) {
        const size_t batchEnd = std::min(batchStart + batchSize, popSize);

        std::vector<std::future<State>> futures;
        futures.reserve(batchEnd - batchStart);

        for (size_t i = batchStart; i < batchEnd; i++) {
            Genome* g = population[i].get();
            futures.push_back(
                std::async(std::launch::async, [this, g]() { return runEpisode(*g); })
            );
        }

        for (size_t i = 0; i < futures.size(); i++) {
            try {
                results[batchStart + i] = futures[i].get();
            } catch (const std::exception& e) {
                if (!fellBack) {
                    std::cerr << "[Trainer] Worker exception: " << e.what()
                              << " - falling back to sequential evaluation.\n";
                    fellBack = true;
                }

                for (size_t j = i + 1; j < futures.size(); j++)
                    try { futures[j].get(); } catch (...) {}

                for (size_t k = batchStart + i; k < popSize; k++)
                    results[k] = runEpisode(*population[k]);

                    return results;
            }
        }

        if (fellBack) break;
    }

    return results;
}

std::vector<State> Trainer::evaluatePopulationSequential(std::vector<std::unique_ptr<Genome>>& population) {
    std::vector<State> results;
    results.reserve(population.size());
    for (auto& genome : population)
        results.push_back(runEpisode(*genome));
    return results;
}

State Trainer::runEpisode(Genome& genome) {
    SnakeEnv env;
    State obs = env.reset();
    bool done = false;

    while (!done) {
        auto action = genome.getAction(obs);
        GymState gs = env.step(action);
        done = gs.done;
        obs = std::move(gs.state);
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
        done = gs.done;
        obs = std::move(gs.state);
    }

    return env.getLastState();
}