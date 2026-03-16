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

void DataBase::purgeExpired() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = expiry.begin(); it != expiry.end();) {
        if (now >= it->second) {
            strings.erase(it->first);
            lists.erase(it->first);
            hashes.erase(it->first);
            it = expiry.erase(it);
        } else {
            ++it;
        }
    }
}

bool DataBase::rename(const std::string &oldKey, const std::string &newKey) {
    std::lock_guard<std::mutex> lock(mtx);
    bool found = false;

    auto it = strings.find(oldKey);
    if (it != strings.end()) {
        strings[newKey] = std::move(it->second);
        strings.erase(it);
        found = true;
    }

    auto il = lists.find(oldKey);
    if (il != lists.end()) {
        lists[newKey] = std::move(il->second);
        lists.erase(il);
        found = true;
    }

    auto ih = hashes.find(oldKey);
    if (ih != hashes.end()) {
        hashes[newKey] = std::move(ih->second);
        hashes.erase(ih);
        found = true;
    }

    auto ie = expiry.find(oldKey);
    if (ie != expiry.end()) {
        expiry[newKey] = ie->second;
        expiry.erase(ie);
    }

    return found;
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

// ─── list operations ──────────────────────────────────────────────

std::vector<std::string> DataBase::lget(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end()) return {};
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

ssize_t DataBase::llen(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    return it != lists.end() ? (ssize_t)it->second.size() : 0;
}

int DataBase::lrem(const std::string &key, int count, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end()) return 0;

    auto &lst = it->second;
    int removed = 0;

    if (count == 0) {
        // remove all occurrences
        for (auto i = lst.begin(); i != lst.end();) {
            if (*i == value) {
                i = lst.erase(i);
                removed++;
            } else
                ++i;
        }
    } else if (count > 0) {
        // remove from head
        for (auto i = lst.begin(); i != lst.end() && removed < count;) {
            if (*i == value) {
                i = lst.erase(i);
                removed++;
            } else
                ++i;
        }
    } else {
        // remove from tail
        for (auto i = lst.rbegin(); i != lst.rend() && removed < -count;) {
            if (*i == value) {
                i = std::reverse_iterator<std::deque<std::string>::iterator>(
                    lst.erase(std::next(i).base()));
                removed++;
            } else
                ++i;
        }
    }
    return removed;
}

bool DataBase::lindex(const std::string &key, int index, std::string &out) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end()) return false;

    auto &lst = it->second;
    if (index < 0) index = (int)lst.size() + index;
    if (index < 0 || index >= (int)lst.size()) return false;

    out = lst[index];
    return true;
}

bool DataBase::lset(const std::string &key, int index, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end()) return false;

    auto &lst = it->second;
    if (index < 0) index = (int)lst.size() + index;
    if (index < 0 || index >= (int)lst.size()) return false;

    lst[index] = value;
    return true;
}

void DataBase::lpush(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    lists[key].push_front(value);
}

void DataBase::rpush(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    lists[key].push_back(value);
}

bool DataBase::lpop(const std::string &key, std::string &out) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end() || it->second.empty()) return false;
    out = it->second.front();
    it->second.pop_front();
    if (it->second.empty()) lists.erase(it);
    return true;
}

bool DataBase::rpop(const std::string &key, std::string &out) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = lists.find(key);
    if (it == lists.end() || it->second.empty()) return false;
    out = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) lists.erase(it);
    return true;
}

// ─── hash operations ──────────────────────────────────────────────

bool DataBase::hexists(const std::string &key, const std::string &field) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = hashes.find(key);
    if (it == hashes.end()) return false;
    return it->second.count(field) > 0;
}

bool DataBase::hdel(const std::string &key, const std::string &field) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = hashes.find(key);
    if (it == hashes.end()) return false;
    return it->second.erase(field) > 0;
}

std::unordered_map<std::string, std::string> DataBase::hgetall(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = hashes.find(key);
    if (it == hashes.end()) return {};
    return it->second;
}

std::vector<std::string> DataBase::hkeys(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> result;
    auto it = hashes.find(key);
    if (it != hashes.end())
        for (auto &[f, v] : it->second)
            result.push_back(f);
    return result;
}

std::vector<std::string> DataBase::hvals(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> result;
    auto it = hashes.find(key);
    if (it != hashes.end())
        for (auto &[f, v] : it->second)
            result.push_back(v);
    return result;
}

ssize_t DataBase::hlen(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = hashes.find(key);
    return it != hashes.end() ? (ssize_t)it->second.size() : 0;
}

void DataBase::hmset(const std::string &key, const std::vector<std::pair<std::string, std::string>> &fieldValues) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto &[field, value] : fieldValues)
        hashes[key][field] = value;
}

void DataBase::hset(const std::string &key, const std::string &field, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    hashes[key][field] = value;
}

bool DataBase::hget(const std::string &key, const std::string &field, std::string &out) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = hashes.find(key);
    if (it == hashes.end()) return false;
    auto fi = it->second.find(field);
    if (fi == it->second.end()) return false;
    out = fi->second;
    return true;
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

    strings.clear();
    lists.clear();
    hashes.clear();

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
