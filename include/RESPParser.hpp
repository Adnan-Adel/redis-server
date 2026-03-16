#pragma once

#include <string>
#include <vector>

class RESPParser {
public:
    void feed(const char *data, size_t len);
    std::vector<std::string> tryParse();

    static std::string encodeSimple(const std::string &str);
    static std::string encodeError(const std::string &err);
    static std::string encodeBulk(const std::string &str);
    static std::string encodeNull();
    static std::string encodeInteger(int n);
    static std::string encodeArray(const std::vector<std::string> &items);

private:
    std::string buffer;
};
