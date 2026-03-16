#include "../include/DataBase.hpp"
#include <fstream>
#include <sstream>

// ─── private helper ───────────────────────────────────────────────

bool DataBase::isExpired(const std::string &key) {
    auto it = expiry.find(key);
    if (it == expiry.end()) return false;
    if (std::chrono::steady_clock::now() >= it->second) {
        expiry.erase(it);
        return true;
    }
    return false;
}

// ─── string operations ────────────────────────────────────────────

void DataBase::set(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    strings[key] = value;
    expiry.erase(key);
}

bool DataBase::get(const std::string &key, std::string &out) {
    std::lock_guard<std::mutex> lock(mtx);
    if (isExpired(key)) {
        strings.erase(key);
        return false;
    }
    auto it = strings.find(key);
    if (it == strings.end()) return false;
    out = it->second;
    return true;
}

bool DataBase::del(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    bool removed = false;
    removed |= strings.erase(key) > 0;
    removed |= lists.erase(key) > 0;
    removed |= hashes.erase(key) > 0;
    expiry.erase(key);
    return removed;
}

bool DataBase::exists(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    if (isExpired(key)) return false;
    return strings.count(key) || lists.count(key) || hashes.count(key);
}

// ─── key operations ───────────────────────────────────────────────

std::string DataBase::type(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    if (isExpired(key)) return "none";
    if (strings.count(key)) return "string";
    if (lists.count(key)) return "list";
    if (hashes.count(key)) return "hash";
    return "none";
}

bool DataBase::expire(const std::string &key, int seconds) {
    std::lock_guard<std::mutex> lock(mtx);
    bool keyExists = strings.count(key) || lists.count(key) || hashes.count(key);
    if (!keyExists) return false;
    expiry[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    return true;
}

std::vector<std::string> DataBase::keys() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> result;
    for (auto &[k, v] : strings)
        if (!isExpired(k)) result.push_back(k);
    for (auto &[k, v] : lists)
        if (!isExpired(k)) result.push_back(k);
    for (auto &[k, v] : hashes)
        if (!isExpired(k)) result.push_back(k);
    return result;
}

void DataBase::flushAll() {
    std::lock_guard<std::mutex> lock(mtx);
    strings.clear();
    lists.clear();
    hashes.clear();
    expiry.clear();
}

// ─── persistence ──────────────────────────────────────────────────

bool DataBase::dump(const std::string &filename) {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    for (auto &[key, value] : strings)
        file << "S " << key << " " << value << "\n";

    for (auto &[key, list] : lists)
        for (auto &item : list)
            file << "L " << key << " " << item << "\n";

    for (auto &[key, hash] : hashes)
        for (auto &[field, value] : hash)
            file << "H " << key << " " << field << " " << value << "\n";

    return file.good();
}

bool DataBase::load(const std::string &filename) {
    std::lock_guard<std::mutex> lock(mtx);
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        char type;
        iss >> type;
        if (type == 'S') {
            std::string key, value;
            iss >> key >> value;
            strings[key] = value;
        } else if (type == 'L') {
            std::string key, item;
            iss >> key >> item;
            lists[key].push_back(item);
        } else if (type == 'H') {
            std::string key, field, value;
            iss >> key >> field >> value;
            hashes[key][field] = value;
        }
    }
    return true;
}
