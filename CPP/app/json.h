// json.h

#pragma once

#include <string>
#include <vector>
#include <cctype>

inline std::string jsonGetRaw(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && std::isspace((unsigned char)json[pos])) pos++;
    if (pos >= json.size()) return "";

    char open = json[pos];
    if (open == '[' || open == '{') {
        char close = (open == '[') ? ']' : '}';
        int depth = 1;
        size_t start = pos;
        pos++;
        while (pos < json.size() && depth > 0) {
            if (json[pos] == open) depth++;
            if (json[pos] == close) depth--;
            pos++;
        }
        return json.substr(start, pos - start);
    } else {
        size_t start = pos;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\n') pos++;
        std::string raw = json.substr(start, pos - start);

        while (!raw.empty() && std::isspace((unsigned char)raw.front())) raw.erase(raw.begin());
        while (!raw.empty() && std::isspace((unsigned char)raw.back())) raw.pop_back();
        if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
            raw = raw.substr(1, raw.size() - 2);
        }
        return raw;
    }
}

inline double jsonGetDouble(const std::string& json, const std::string& key, double def = 0.0) {
    auto s = jsonGetRaw(json, key);
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

inline int jsonGetInt(const std::string& json, const std::string& key, int def = 0) {
    auto s = jsonGetRaw(json, key);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

inline std::string jsonGetStr(const std::string& json, const std::string& key, const std::string& def = "") {
    auto s = jsonGetRaw(json, key);
    return s.empty() ? def : s;
}

inline std::vector<int> jsonParseIntArray(const std::string& arr) {
    std::vector<int> out;
    bool inside = false;
    std::string cur;
    for (char c : arr) {
        if (c == '[') { inside = true; continue; }
        if (c == ']') { if (!cur.empty()) { try { out.push_back(std::stoi(cur)); } catch (...) {} cur.clear(); } break; }
        if (inside && (std::isdigit((unsigned char)c) || c == '-')) cur += c;
        else if (inside && c == ',') { if (!cur.empty()) { try { out.push_back(std::stoi(cur)); } catch (...) {} cur.clear(); } }
    }
    return out;
}
