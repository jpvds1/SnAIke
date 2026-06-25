// menu.h

#pragma once

#include <memory>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <optional>
#include <string>
#include <vector>

#include "configRegistry.h"
#include "agent.h"
#include "registry.h"
#include "trainer.h"

struct MenuResult {
    std::unique_ptr<Agent> agent;
    TrainerConfig          config;
};

MenuResult runMenu(const std::vector<AgentEntry>& registry);