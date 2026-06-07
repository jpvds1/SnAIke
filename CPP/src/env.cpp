#include "../include/env.h"

State SnakeEnv::reset() {
    lastState = engine.reset();
    return lastState;
}

GymState SnakeEnv::step(std::optional<Direction> dir) {
    GymState retState = engine.update(dir);
    lastState = retState.state;
    return retState;
}

State SnakeEnv::getLastState() const {
    return lastState;
}