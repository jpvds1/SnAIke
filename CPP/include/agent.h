// agent.h

#pragma once

#include <string>
#include <vector>
#include <optional>

#include "types.h"

class Genome {
public:
    virtual ~Genome() = default;
    virtual std::optional<Direction> getAction(State state) = 0;
};


class Agent {
public:
    virtual ~Agent() = default;

    virtual std::optional<Direction> getAction(State state) = 0;
    static std::vector<std::string> loadStats();
};


class PopulationAgent {
public:
    virtual ~PopulationAgent() = default;

    virtual std::vector<std::unique_ptr<Genome>>& getPopulation() = 0;
    virtual void evolve(std::vector<State> results) = 0;
};