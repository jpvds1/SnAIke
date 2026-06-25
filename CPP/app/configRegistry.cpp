#include "configRegistry.h"

static constexpr const char* CKPT_DIR = "../checkpoints";

// ---------------------------------------------------------------------------
// local helpers
// ---------------------------------------------------------------------------

static std::string nowString() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

static std::string serializeParams(const ConfigParams& p) {
    std::ostringstream o;
    o << "{";
    bool first = true;
    for (const auto& [k, v] : p) {
        if (!first) o << ", ";
        first = false;
        o << "\"" << k << "\": \"" << v << "\"";
    }
    o << "}";
    return o.str();
}

static ConfigParams parseParams(const std::string& obj) {
    ConfigParams out;
    size_t i = 0;
    auto skipWs = [&]() { while (i < obj.size() && std::isspace((unsigned char)obj[i])) i++; };

    skipWs();
    if (i < obj.size() && obj[i] == '{') i++;
    while (i < obj.size()) {
        skipWs();
        if (i < obj.size() && obj[i] == '}') break;
        if (i >= obj.size() || obj[i] != '"') { i++; continue; }
        i++;
        std::string key;
        while (i < obj.size() && obj[i] != '"') key += obj[i++];
        if (i < obj.size()) i++;
        skipWs();
        if (i < obj.size() && obj[i] == ':') i++;
        skipWs();
        std::string val;
        if (i < obj.size() && obj[i] == '"') {
            i++;
            while (i < obj.size() && obj[i] != '"') val += obj[i++];
            if (i < obj.size()) i++;
        } else {
            while (i < obj.size() && obj[i] != ',' && obj[i] != '}') val += obj[i++];
        }
        out[key] = val;
        skipWs();
        if (i < obj.size() && obj[i] == ',') i++;
    }
    return out;
}

static std::vector<std::string> splitObjects(const std::string& arr) {
    std::vector<std::string> out;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < arr.size(); i++) {
        char c = arr[i];
        if (c == '{') { if (depth == 0) start = i; depth++; }
        else if (c == '}') { depth--; if (depth == 0) out.push_back(arr.substr(start, i - start + 1)); }
    }
    return out;
}

// ---------------------------------------------------------------------------
// ConfigRegistry
// ---------------------------------------------------------------------------

ConfigRegistry::ConfigRegistry(std::string agentName) : agentName(std::move(agentName)) {
    loadFile();
}

std::string ConfigRegistry::registryPath() const {
    return std::string(CKPT_DIR) + "/" + agentName + "_registry.json";
}

std::string ConfigRegistry::weightsPath(int id) const {
    return std::string(CKPT_DIR) + "/" + agentName + "_" + std::to_string(id) + "_weights.bin";
}

void ConfigRegistry::loadFile() {
    std::ifstream f(registryPath());
    if (!f.is_open()) return;

    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string configsArr = jsonGetRaw(json, "configs");
    for (const auto& obj : splitObjects(configsArr)) {
        ConfigRecord r;
        r.id = jsonGetInt(obj, "id");
        r.timestamp = jsonGetStr(obj, "timestamp");
        r.generation = jsonGetInt(obj, "generation");
        r.bestScore = jsonGetInt(obj, "best_score");
        r.params = parseParams(jsonGetRaw(obj, "params"));
        records.push_back(std::move(r));
    }
}

void ConfigRegistry::saveFile() const {
    std::filesystem::create_directories(CKPT_DIR);
    std::ofstream f(registryPath());
    f << "{\n  \"agent\": \"" << agentName << "\",\n  \"configs\": [\n";
    for (size_t i = 0; i < records.size(); i++) {
        const auto& r = records[i];
        f << "    {\n"
          << "      \"id\": " << r.id << ",\n"
          << "      \"timestamp\": \"" << r.timestamp << "\",\n"
          << "      \"generation\": " << r.generation << ",\n"
          << "      \"best_score\": " << r.bestScore << ",\n"
          << "      \"params\": " << serializeParams(r.params) << "\n"
          << "    }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    f << "  ]\n}\n";
}

std::optional<ConfigRecord> ConfigRegistry::findByParams(const ConfigParams& params) const {
    for (const auto& r : records)
        if (r.params == params) return r;
    return std::nullopt;
}

ConfigRecord ConfigRegistry::createNew(const ConfigParams& params) {
    int maxId = 0;
    for (const auto& r : records) maxId = std::max(maxId, r.id);

    ConfigRecord r;
    r.id = maxId + 1;
    r.timestamp = nowString();
    r.params = params;
    r.generation = 0;
    r.bestScore = 0;
    records.push_back(r);
    saveFile();
    return r;
}

void ConfigRegistry::update(int id, int generation, int bestScore) {
    for (auto& r : records) {
        if (r.id == id) {
            r.generation = generation;
            r.bestScore = bestScore;
            saveFile();
            return;
        }
    }
}
