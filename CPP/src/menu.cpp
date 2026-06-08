#include "../include/menu.h"
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
            params[key] = promptString(key, params[key]);
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
// Training limit selection
// --------------------------------------------------------------------------------------

static TrainerConfig selectTrainerConfig() {
    TrainerConfig config;

    while (true) {
        printSeparator();
        std::cout << "Training limit\n\n"
                  << "  [1] Generation limit\n"
                  << "  [2] Time limit (minutes)\n"
                  << "  [3] both\n\n"
                  << "Choice: ";
        int c;
        if (std::cin >> c && c >= 1 && c <= 3) {
            clearInput();

            if (c == 1 || c == 3) {
                while (true) {
                    std::cout << "  Max generations [100]: ";
                    std::string line;
                    std::getline(std::cin, line);
                    if (line.empty()) { 
                        config.maxGenerations = 100; 
                        break; 
                    }
                    try {
                        int n = std::stoi (line);
                        if (n > 0) {
                            config.maxGenerations = n;
                            break;
                        }
                    } catch (...) {}
                    std::cout << "  Please enter a positive integer.\n";
                }
            }

            if (c == 2 || c == 3) {
                while (true) {
                    std::cout << "  Time limit in minutes [10.0]: ";
                    std::string line;
                    std::getline(std::cin, line);
                    if (line.empty()) {
                        config.timeLimitMinutes = 10.0;
                        break;
                    }
                    try {
                        double t = std::stod(line);
                        if (t > 0.0) {
                            config.timeLimitMinutes = t;
                            break;
                        }
                    } catch (...) {}
                    std::cout << "  Plea enter a positive number.\n";
                }
            }
            break;
        }
        clearInput();
        std::cout << "  Invalid choice, try again.\n";
    }
    return config;
}

// --------------------------------------------------------------------------------------
// Public
// --------------------------------------------------------------------------------------

MenuResult runMenu(const std::vector<AgentEntry> &registry) {
    std::cout << "\n======== Snake Training ========\n";

    const AgentEntry& entry  = selectAgent(registry);
    ParamMap          params = editParams(entry);
    TrainerConfig     config = selectTrainerConfig();

    printSeparator();

    std::cout << "\nStarting training: " << entry.name << "\n";
    if (config.maxGenerations)
        std::cout << "  Max generations: " << *config.maxGenerations << "\n";
    if (config.timeLimitMinutes)
        std::cout << "  Time limit: " << *config.timeLimitMinutes << " min\n";

    return { entry.factory(params), config };
}