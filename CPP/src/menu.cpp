#include "../include/menu.h"
#include <ios>
#include <limits>
#include <memory>
#include <string>

// --------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------

static void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static void printSeparator() {
    std::cout << "\n------------------------------------------------\n";
}

static std::string promptString(const std::string& label, const std::string& def) {
    std::cout << " " << label << " [" << def << "]: ";
    std::string line;
    std::getline(std::cin, line);
    return line.empty() ? def : line;
}

// --------------------------------------------------------------------------------------
// Agent Selection
// --------------------------------------------------------------------------------------

static const AgentEntry& selectAgent(const std::vector<AgentEntry>& registry) {
    while (true) {
        printSeparator();
        std::cout << "Available agents\n\n";
        for (int i = 0; i < registry.size(); i++) {
            std::cout << " [" << (i + 1) << " ]" << registry[i].name
                      << " - " << registry[i].description << "\n";
        }
        std::cout << "\nSelect  agent: ";
        int choice;
        if (std::cin >> choice && choice >= 1 && choice <= (int)registry.size()) {
            clearInput();
            return registry[choice - 1];
        }
        clearInput();
        std::cout << " Invalid choice, try again.\n";
    }
}

// --------------------------------------------------------------------------------------
// Hyperparameter editing
// --------------------------------------------------------------------------------------

static ParamMap editParams(const AgentEntry& entry) {
    ParamMap params;
    for (const auto& [key, def] : entry.paramDefs)
        params[key] = def;

    while (true) {
        printSeparator();
        std::cout << "Hyperparameters for \"" << entry.name << "\" (Enter to keep default):\n\n";
        for (const auto& [key, def] : entry.paramDefs) {
            std::string val = promptString(key, params[key]);
            params[key] = val;
        }

        printSeparator();
        std::cout << "Current settings:\n\n";
        for (const auto& [key, val] : params)
            std::cout << " " << key << " = " << val << "\n";

        std::cout << "\n [1] Accept\n [2] Edit again\n\nChoice: ";
        int c;
        if (std::cin >> c) {
            clearInput();
            if (c == 1) break;
        } else {
            clearInput();
        }
    }
    return params;
}

// --------------------------------------------------------------------------------------
// Training settings
// --------------------------------------------------------------------------------------

static int selectGenerations() {
    while (true) {
        printSeparator();
        std::cout << "How many generations to train? [100]: ";
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) return 100;
        try {
            int n = std::stoi(line);
            if (n > 0) return n;
        } catch (...) {}
        std::cout << " Please enter a positive integer.\n";
    }
}

// --------------------------------------------------------------------------------------
// Public
// --------------------------------------------------------------------------------------

std::unique_ptr<PopulationAgent> runMenu(
    const std::vector<AgentEntry>& registry,
    int& trainGenerations
) {
    std::cout << "\n======== Snake Training ========\n";

    const AgentEntry& entry = selectAgent(registry);
    ParamMap params = editParams(entry);
    trainGenerations = selectGenerations();

    printSeparator();
    std::cout << "\nStarting training: " << entry.name
              << " for " << trainGenerations << " generations.\n\n";

    return entry.factory(params);
}