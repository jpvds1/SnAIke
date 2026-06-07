// session.g

#pragma once

#include <vector>
#include <optional>
#include <memory>

#include "agent.h"
#include "env.h"
#include "types.h"

class Session {
public:
    Session(
        std::unique_ptr<Agent> agent,
        SnakeEnv env    
    );

    RunResult run();
private:
    std::unique_ptr<Agent> agent;
    SnakeEnv env;
};