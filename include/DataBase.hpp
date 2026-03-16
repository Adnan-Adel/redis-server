#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <chrono>

class DataBase {
public:
    DataBase() = default;
    ~DataBase() = default;

    DataBase(const DataBase &) = delete;
    DataBase &operator=(const DataBase &) = delete;

    // persistence
    bool dump(const std::string &filename);
    bool load(const std::string &filename);

    // string operations
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &out);
    bool del(const std::string &key);
    bool exists(const std::string &key);

    // key operations
    std::string type(const std::string &key);
    bool expire(const std::string &key, int seconds);
    std::vector<std::string> keys();
    void flushAll();

private:
    bool isExpired(const std::string &key);

    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, std::deque<std::string>> lists;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashes;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry;
    std::mutex mtx;
};
