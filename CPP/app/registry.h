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

    std::vector<std::pair<std::string, bool>> inputParamDefs;

    std::vector<std::pair<std::string, std::string>> fitnessFunctionDefs;
    std::string defaultFitnessFunction;

    std::function<std::unique_ptr<Agent>(const ParamMap&)> factory;
};

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

inline int paramInt (const ParamMap& p, const std::string& k, int def) {
    auto it = p.find(k); 
    return (it != p.end() && !it->second.empty()) ? std::stoi(it->second) : def;
}

inline double paramDouble(const ParamMap& p, const std::string& k, double def) {
    auto it = p.find(k); 
    return (it != p.end() && !it->second.empty()) ? std::stod(it->second) : def;
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
                {"save_interval",         "20"},
                {"hidden_layers",         "[16]"}
            },
            .inputParamDefs = {
                {"relative_apple",     true},
                {"absolute_apple",     false},
                {"head_position",      false},
                {"direction",          true},
                {"snake_size",         true},
                {"distance_to_walls",  true},
                {"distance_to_danger", true},
                {"danger_flags",       false},
                {"full_grid",          false}
            },
            .fitnessFunctionDefs = {
                {"score_steps", "Default - rewards apples eaten, penalizes slow food-seeking"},
                {"survival",    "Survival - rewards steps survived, scaled by score"},
                {"efficiency",  "Efficiency - strongly rewards apples eaten per step"}
            },
            .defaultFitnessFunction = "score_steps",
            .factory = [](const ParamMap& p) -> std::unique_ptr<Agent> {
                return std::make_unique<GeneticAgent>("genetic", p);
            }
        }
    };
}