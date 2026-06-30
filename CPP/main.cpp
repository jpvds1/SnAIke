#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "registry.h"
#include "menu.h"
#include "agent.h"
#include "trainer.h"
#include "helpers.h"

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

    std::cout << "  Input parameters: input_components=<comma,separated>\n"
              << "    available: ";
    bool first = true;
    for (const auto& name : availableInputComponents()) {
        std::cout << (first ? "" : ",") << name;
        first = false;
    }
    std::cout << "\n\n";

    std::cout <<
        "  Training limits (at least on required):\n"
        "    generations=<N>     stop after N generation\n"
        "    time_minutes=<F>    stop after F minutes\n\n"
        "  Environment:\n"
        "    env_width=20\n"
        "    env_height=20\n\n"
        "  Parallelism:\n"
        "    workers=<N>         parallel evaluation workers (0 = auto)\n\n"
        "  Example:\n"
        "    snake genetic generations=200 mutation_rate=0.15\n"
        "    snake genetic time_minutes=5 workers=8\n"
        "    snake genetic generations=100 time_minutes=2\n"
        "    snake genetic generations=200 input_components=relative_apple,direction,snake_size\n";
}

static bool parseArgs(int argc, char* argv[], int startIdx, ParamMap& params, std::string& reason) {
    for (int i = startIdx; i < argc; i++) {
        std::string token(argv[i]);
        auto eq = token.find("=");
        if (eq == std::string::npos) {
            reason = "malformed argument \"" + token + "\" (expected key=value)";
            return false;
        }
        params[token.substr(0, eq)] = token.substr(eq + 1);
    }
    return true;
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
    if (auto it = params.find("workers"); it != params.end()) {
        config.nWorkers = std::stoi(it->second);
        params.erase(it);
    }

    return config;
}

static bool buildFromArgs(const std::vector<AgentEntry>& registry,
                          int argc, char* argv[],
                          std::unique_ptr<Agent>& agent,
                          TrainerConfig& config,
                          std::string& reason) {
    std::string agentName(argv[1]);

    const AgentEntry* entry = nullptr;
    for (const auto& e : registry)
        if (e.name == agentName) { entry = &e; break; }
    if (!entry) {
        reason = "unknown agent \"" + agentName + "\"";
        return false;
    }

    ParamMap params;
    if (!parseArgs(argc, argv, 2, params, reason))
        return false;

    config = buildConfig(params);
    if (!config.maxGenerations && !config.timeLimitMinutes) {
        reason = "no training limit (specify generations=<N> or time_minutes=<F>)";
        return false;
    }

    if (auto it = params.find("input_components"); it != params.end()) {
        const auto& available = availableInputComponents();
        std::string cur;
        auto check = [&](const std::string& name) -> bool {
            if (name.empty()) return true;
            if (std::find(available.begin(), available.end(), name) != available.end())
                return true;
            reason = "unknown input component \"" + name + "\"";
            return false;
        };
        for (char c : it->second) {
            if (c == ',') { if (!check(cur)) return false; cur.clear(); }
            else cur += c;
        }
        if (!check(cur)) return false;
    }

    try {
        agent = entry->factory(params);
    } catch (const std::exception& e) {
        reason = std::string("invalid parameter value (") + e.what() + ")";
        return false;
    }

    std::cout << "[main] Agent: " << entry->name << "\n";
    if (config.maxGenerations)
        std::cout << "  Max generations: " << *config.maxGenerations << "\n";
    if (config.timeLimitMinutes)
        std::cout << "  Time limit:" << *config.timeLimitMinutes << "\n";
    std::cout << "  Workers: " << config.resolvedWorkers() << "\n";
    for (const auto& [k, v] : params)
        std::cout << "  " << k << " = " << v << "\n";
    std::cout << "\n";

    return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    auto registry = makeRegistry();

    std::unique_ptr<Agent> agent;
    TrainerConfig          config;

    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(registry);
        return 0;
    }

    if (argc == 1) {
        MenuResult result = runMenu(registry);
        agent  = std::move(result.agent);
        config = result.config;
    } else {
        std::string reason;
        if (!buildFromArgs(registry, argc, argv, agent, config, reason)) {
            std::cerr << "[main] " << reason << "; opening menu.\n";
            return 1;
        }
    }

    Trainer trainer(*agent, config);
    trainer.run();

    return 0;
}