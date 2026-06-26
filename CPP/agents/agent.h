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
    virtual void train(std::vector<State> results) = 0;

    virtual void atTrainingEnd() {}
};


class PopulationAgent : public Agent {
public:
    virtual ~PopulationAgent() = default;

    virtual std::vector<std::unique_ptr<Genome>>& getPopulation() = 0;
    void train(std::vector<State> results) override { evolve(std::move(results)); }
    virtual void evolve(std::vector<State> results) = 0;
};