// configRegistry.h

#pragma once

#include "json.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

using ConfigParams = std::map<std::string, std::string>;

struct ConfigRecord {
    int id = 0;
    std::string timestamp;
    ConfigParams params;
    int generation = 0;
    int bestScore = 0;
};

class ConfigRegistry {
public:
    explicit ConfigRegistry(std::string agentName);

    std::vector<ConfigRecord> list() const { return records; }
    std::optional<ConfigRecord> findByParams(const ConfigParams& params) const;

    ConfigRecord createNew(const ConfigParams& params);
    void update(int id, int generation, int bestScore);

    std::string weightsPath(int id) const;

private:
    std::string agentName;
    std::vector<ConfigRecord> records;

    std::string registryPath() const;
    void loadFile();
    void saveFile() const;
};
