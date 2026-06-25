#include "menu.h"

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

static void editHyperparams(const AgentEntry& entry, ParamMap& params) {
    while (true) {
        printSeparator();
        std::cout << "Hyperparameters for \"" << entry.name << "\" (Enter to keep current):\n\n";
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
}

// --------------------------------------------------------------------------------------
// Input parameter selection
// --------------------------------------------------------------------------------------

static void toggleInputs(const AgentEntry& entry, std::vector<bool>& enabled) {
    while (true) {
        printSeparator();
        std::cout << "Input parameters (type a number to toggle, Y when done):\n\n";
        for (size_t i = 0; i < entry.inputParamDefs.size(); i++) {
            std::cout << "  [" << (enabled[i] ? 'X' : ' ') << "] " << (i + 1)
                      << ") " << entry.inputParamDefs[i].first << "\n";
        }
        std::cout << "\nChoice: ";
        std::string line;
        std::getline(std::cin, line);
        if (line == "Y" || line == "y") return;
        try {
            int n = std::stoi(line);
            if (n >= 1 && n <= (int)enabled.size()) enabled[n - 1] = !enabled[n - 1];
        } catch (...) {}
    }
}

static std::string joinEnabled(const AgentEntry& entry, const std::vector<bool>& enabled) {
    std::string out;
    for (size_t i = 0; i < entry.inputParamDefs.size(); i++) {
        if (!enabled[i]) continue;
        if (!out.empty()) out += ",";
        out += entry.inputParamDefs[i].first;
    }
    return out;
}

// --------------------------------------------------------------------------------------
// New / previous configuration
// --------------------------------------------------------------------------------------

static ParamMap buildNewConfig(const AgentEntry& entry) {
    ParamMap params;
    for (const auto& [key, def] : entry.paramDefs) params[key] = def;

    std::vector<bool> enabled;
    for (const auto& [name, def] : entry.inputParamDefs) enabled.push_back(def);

    while (true) {
        printSeparator();
        std::cout << "New configuration\n\n"
                  << "  [1] Modify input parameters\n"
                  << "  [2] Modify hyperparameters\n"
                  << "  [3] Done\n\nChoice: ";
        int c;
        if (!(std::cin >> c)) { clearInput(); continue; }
        clearInput();

        if (c == 1) {
            toggleInputs(entry, enabled);
        } else if (c == 2) {
            editHyperparams(entry, params);
        } else if (c == 3) {
            std::string comp = joinEnabled(entry, enabled);
            if (comp.empty()) {
                std::cout << "  Select at least one input parameter.\n";
                continue;
            }
            params["input_components"] = comp;
            return params;
        }
    }
}

static std::optional<ParamMap> selectPrevious(ConfigRegistry& reg) {
    auto records = reg.list();
    if (records.empty()) {
        std::cout << "\n  No previous configurations found.\n";
        return std::nullopt;
    }

    while (true) {
        printSeparator();
        std::cout << "Previous configurations:\n\n";
        for (size_t i = 0; i < records.size(); i++) {
            const auto& r = records[i];
            std::cout << "  [" << (i + 1) << "] #" << r.id << "  " << r.timestamp
                      << "  gen=" << r.generation << "  best=" << r.bestScore << "\n";
        }
        std::cout << "\n  [0] Back\n\nSelect configuration: ";
        int c;
        if (!(std::cin >> c)) { clearInput(); continue; }
        clearInput();
        if (c == 0) return std::nullopt;
        if (c < 1 || c > (int)records.size()) continue;

        const auto& r = records[c - 1];
        printSeparator();
        std::cout << "Configuration #" << r.id << " (gen=" << r.generation
                  << ", best=" << r.bestScore << ")\n\n";
        for (const auto& [k, v] : r.params)
            std::cout << "  " << k << " = " << v << "\n";

        std::cout << "\n  [1] Use this\n  [2] Back\n\nChoice: ";
        int d;
        if (std::cin >> d) {
            clearInput();
            if (d == 1) return r.params;
        } else {
            clearInput();
        }
    }
}

static ParamMap selectConfiguration(const AgentEntry& entry) {
    ConfigRegistry reg(entry.name);
    while (true) {
        printSeparator();
        std::cout << "Configuration\n\n"
                  << "  [1] Previous configuration\n"
                  << "  [2] New configuration\n\nChoice: ";
        int c;
        if (!(std::cin >> c)) { clearInput(); continue; }
        clearInput();

        if (c == 1) {
            if (auto p = selectPrevious(reg)) return *p;
        } else if (c == 2) {
            return buildNewConfig(entry);
        }
    }
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

    printSeparator();
    const int hw = config.resolvedWorkers();
    std::string def = std::to_string(hw);
    std::cout << "Parallel workers\n\n";
    while (true) {
        std::string val = promptString("Workers (0 = auto)", def);
        try {
            int n = std::stoi(val);
            if (n >= 0) { config.nWorkers = n; break; }
        } catch (...) {}
        std::cout << "  Please enter a non-negative integer.\n";
    }

    return config;
}

// --------------------------------------------------------------------------------------
// Public
// --------------------------------------------------------------------------------------

MenuResult runMenu(const std::vector<AgentEntry> &registry) {
    std::cout << "\n======== Snake Training ========\n";

    const AgentEntry& entry  = selectAgent(registry);
    ParamMap          params = selectConfiguration(entry);
    TrainerConfig     config = selectTrainerConfig();

    printSeparator();

    std::cout << "\nStarting training: " << entry.name << "\n";
    if (config.maxGenerations)
        std::cout << "  Max generations: " << *config.maxGenerations << "\n";
    if (config.timeLimitMinutes)
        std::cout << "  Time limit: " << *config.timeLimitMinutes << " min\n";

    return { entry.factory(params), config };
}