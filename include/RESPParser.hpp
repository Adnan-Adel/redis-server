#pragma once

#include <string>
#include <vector>

namespace RESP {
    std::vector<std::string> parse(const std::string &input);
    std::pair<std::vector<std::string>, size_t> tryParse(const std::string &buffer);
    std::string encode(const std::string &response);
}
