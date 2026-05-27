// env.h

#pragma once

#include <optional>

#include "types.h"
#include "engine.h"

class SnakeEnv {
public:
    SnakeEnv();
    
    State reset();
    GymState step(std::optional<Direction> dir);
    State getLastState() const;
private:
    Engine engine;
    State lastState;
};