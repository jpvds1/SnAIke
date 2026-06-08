#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "./include/registry.h"
#include "./include/menu.h"
#include "./include/env.h"
#include "include/agent.h"
#include "include/types.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void printUsage(const std::vector<AgentEntry>& registry) {
    std::cout << "Usage:\n"
              << "  snake                         - interactive menu\n"
              << "  snake <agent> [key=value ...] - train directly\n\n"
              << "Available agents:\n";
    for (const auto& e: registry) {
        std::cout << "  " << e.name << "\n    " << e.description << "\n\n"
                  << "     Params (key=default):\n";
    }
    for (const auto& e : registry)
        for (const auto& [k, def] : e.paramDefs)
            std::cout << "      " << k << "=" << def << "\n";
    std::cout << "        generations=100\n";
}

static ParamMap parseArgs(int argc, char* argv[], int startIdx) {
    ParamMap params;
    for (int i = startIdx; i < argc; i++) {
        std::string token(argv[i]);
        auto eq = token.find("=");
        if (eq == std::string::npos) {
            std::cerr << "[main] Warning: ignoring malformed argument \"" << token
                    << "\" (expected key=value)\n";
            continue;
        }
        params[token.substr(0, eq)] = token.substr(eq + 1);
    }
    return params;
}

// ---------------------------------------------------------------------------
// Training loop
// ---------------------------------------------------------------------------

static void train(PopulationAgent& agent, int trainGenerations) {
    for (int gen = 0; gen < trainGenerations; gen++) {
        auto& population = agent.getPopulation();
        std::vector<State> results;
        results.reserve(population.size());

        for (auto& genome : population) {
            SnakeEnv env;
            State obs = env.reset();
            bool done = false;

            while (!done) {
                auto action = genome->getAction(obs);
                GymState gs = env.step(action);
                obs = gs.state;
                done = gs.done;
            }
            results.push_back(env.getLastState());
        }

        agent.evolve(results);
    }

    std::cout << "[main] Training complete.\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    auto registry = makeRegistry();

    std::unique_ptr<PopulationAgent> agent;
    int trainGenerations = 100;

    if (argc == 1) {
        agent = runMenu(registry, trainGenerations);
    } else {
        std::string agentName(argv[1]);

        if (agentName == "--help" || agentName == "-h") {
            printUsage(registry);
            return 0;
        }

        const AgentEntry* entry = nullptr;
        for (const auto& e : registry)
            if (e.name == agentName) { entry = &e; break; }

        if (!entry) {
            std::cerr << "[main] Unknown agent \"" << agentName << "\".\n\n";
            printUsage(registry);
            return 1;
        }

        ParamMap params = parseArgs(argc, argv, 2);
        trainGenerations = paramInt(params, "generations", 100);
        params.erase("generations");

        std::cout << "[main] Agent: " << entry->name
                  << " | Generations: " << trainGenerations << "\n";
        for (const auto& [k, v] : params)
            std::cout << "  " << k << " = " << v << "\n";
        std::cout << "\n";

        agent = entry->factory(params);
    }

    train(*agent, trainGenerations);
    return 0;
}