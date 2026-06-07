#include "../include/session.h"

Session::Session(std::unique_ptr<Agent> agent, SnakeEnv env) 
    : agent(std::move(agent)), env(env) {}

RunResult Session::run() {
    State obs = env.reset();
    bool done = false;
    float totalReward = 0.0;
    bool aborted = false;

    while (!done) {
        std::optional<Direction> action = agent->getAction(obs);
        GymState gymState = env.step(action);
        totalReward += gymState.reward;
        done = gymState.done;
    }

    State finalState = env.getLastState();
    return {
        .totalReward = totalReward,
        .score = finalState.score,
        .steps = finalState.steps,
        .aborted = aborted
    };
}