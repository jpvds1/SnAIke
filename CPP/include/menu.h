// menu.h

#pragma once

#include <memory>
#include "agent.h"
#include "registry.h"

std::unique_ptr<PopulationAgent> runMenu(
    const std::vector<AgentEntry>& registry,
    int& trainGenerations
);