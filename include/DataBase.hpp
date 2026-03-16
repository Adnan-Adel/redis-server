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
    bool rename(const std::string &oldKey, const std::string &newKey);
    std::vector<std::string> keys();
    void flushAll();

    // list operations
    void lpush(const std::string &key, const std::string &value);
    void rpush(const std::string &key, const std::string &value);
    bool lpop(const std::string &key, std::string &out);
    bool rpop(const std::string &key, std::string &out);
    std::vector<std::string> lget(const std::string &key);
    ssize_t llen(const std::string &key);
    int lrem(const std::string &key, int count, const std::string &value);
    bool lindex(const std::string &key, int index, std::string &out);
    bool lset(const std::string &key, int index, const std::string &value);

    // hash operations
    void hset(const std::string &key, const std::string &field, const std::string &value);
    bool hget(const std::string &key, const std::string &field, std::string &out);
    bool hexists(const std::string &key, const std::string &field);
    bool hdel(const std::string &key, const std::string &field);
    std::unordered_map<std::string, std::string> hgetall(const std::string &key);
    std::vector<std::string> hkeys(const std::string &key);
    std::vector<std::string> hvals(const std::string &key);
    ssize_t hlen(const std::string &key);
    void hmset(const std::string &key, const std::vector<std::pair<std::string, std::string>> &fieldValues);

private:
    // must be called with mtx already held
    bool isExpired(const std::string &key);
    void purgeExpired();

    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, std::deque<std::string>> lists;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashes;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry;
    std::mutex mtx;
};
