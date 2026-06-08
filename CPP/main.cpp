#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "./include/registry.h"
#include "./include/menu.h"
#include "./include/env.h"
#include "include/agent.h"
#include "include/trainer.h"
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
        for (const auto& [k, def] : e.paramDefs)
            std::cout << "      " << k << "=" << def << "\n";
        std::cout << "\n";
    }

    std::cout <<
        "  Training limits (at least on required):\n"
        "    generations=<N>     stop after N generation\n"
        "    time_minutes=<F>    stop after F minutes\n\n"
        "  Environment:\n"
        "    env_width=20\n"
        "    env_height=20\n\n"
        "  Example:\n"
        "    snake genetic generations=200 mutation_rate=0.15\n"
        "    snake genetic time_minutes=5\n"
        "    snake genetic generations=100 time_minutes=2\n";
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

static TrainerConfig buildConfig(ParamMap& params) {
    TrainerConfig config;

    auto genIt  = params.find("generations");
    auto timeIt = params.find("time_minutes");

    if (genIt != params.end()) {
        config.maxGenerations = std::stoi(genIt->second);
        params.erase(genIt);
    }
    if (timeIt != params.end()) {
        config.timeLimitMinutes = std::stod(timeIt->second);
        params.erase(timeIt);
    }

    if (auto it = params.find("env_width"); it != params.end()) {
        config.envWidth = std::stoi(it->second);
        params.erase(it);
    }
    if (auto it = params.find("env_height"); it != params.end()) {
        config.envHeight = std::stoi(it->second);
        params.erase(it);
    }

    return config;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    auto registry = makeRegistry();

    std::unique_ptr<Agent> agent;
    TrainerConfig                    config;

    if (argc == 1) {
        MenuResult result = runMenu(registry);
        agent = std::move(result.agent);
        config = result.config;
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
        config = buildConfig(params);

        if (!config.maxGenerations && !config.timeLimitMinutes) {
            std::cerr << "[main] Error: specify at least one of "
                         "generations=<N> or time_minutes=<F>.\n\n";
            printUsage(registry);
            return 1;
        }

        std::cout << "[main] Agent: " << entry->name << "\n";
        if (config.maxGenerations)
            std::cout << "  Max generations: " << *config.maxGenerations << "\n";
        if (config.timeLimitMinutes)
            std::cout << "  Time limit:" << *config.timeLimitMinutes << "\n";
        for (const auto& [k, v] : params)
            std::cout << "  " << k << " = " << v << "\n";
        std::cout << "\n";

        agent = entry->factory(params);
    }

    Trainer trainer(*agent, config);
    trainer.run();
    
    return 0;
}