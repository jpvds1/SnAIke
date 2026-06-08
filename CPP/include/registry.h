// registry.h

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "agent.h"
#include "geneticAgent.h"

using ParamMap = std::map<std::string, std::string>;

struct AgentEntry {
    std::string name;
    std::string description;

    std::vector<std::pair<std::string, std::string>> paramDefs;

    std::function<std::unique_ptr<PopulationAgent>(const ParamMap&)> factory;
};

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

inline int paramInt (const ParamMap& p, const std::string& k, int def) {
    auto it = p.find(k); return (it != p.end() && !it->second.empty()) ? std::stoi(it->second) : def;
}

inline double paramDouble(const ParamMap& p, const std::string& k, double def) {
    auto it = p.find(k); return (it != p.end() && !it->second.empty()) ? std::stod(it->second) : def;
}

// ------------------------------------------------------------
// Registry
// ------------------------------------------------------------

inline std::vector<AgentEntry> makeRegistry() {
    return {
        {
            .name = "genetic",
            .description = "Genetic Algorithm - evolves a fixed-topology neural network",
            .paramDefs = {
                {"pop_size",              "200"},
                {"elite_count",           "5"},
                {"mutation_rate",         "0.10"},
                {"mutation_strength",     "0.20"},
                {"min_mutation_strength", "0.05"},
                {"mut_stren_dropoff",     "40"},
                {"tournament_size",       "5"},
                {"save_interval",         "20"}
            },
            .factory = [](const ParamMap& p) -> std::unique_ptr<PopulationAgent> {
                return std::make_unique<GeneticAgent>(
                    paramInt   (p, "pop_size",              200),
                    paramInt   (p, "elite_count",           5),
                    paramDouble(p, "mutation_rate",         0.10),
                    paramDouble(p, "mutation_strength",     0.20),
                    paramDouble(p, "min_mutation_strength", 0.05),
                    paramInt   (p, "mut_stren_dropoff",     40),
                    paramInt   (p, "tournament_size",       5),
                    paramInt   (p, "save_interval",         20)
                );
            }
        }
    };
}