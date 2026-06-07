// engine.h

#pragma once

#include <tuple>
#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <optional>
#include <random>
#include <unordered_set>

#include "snake.h"
#include "helpers.h"
#include "types.h"

constexpr float REWARD_APPLE = 1.0f;
constexpr float REWARD_STEP = 0.0f;
constexpr float REWARD_DEATH = -1.0f;

class Engine {
public:
    Engine();
    
    State reset();
    GymState update(std::optional<Direction> dir = std::nullopt);
    State getState() const;

private:
    int width;
    int height;
    State state;
    Snake snake;

    Position nextHead(std::optional<Direction> dir);
    bool isDead() const;
    std::optional<Position> spawnApple();
};