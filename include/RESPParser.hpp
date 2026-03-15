#pragma once

#include <string>
#include <vector>

namespace RESP {
    std::vector<std::string> parse(const std::string &input);

    std::string encode(const std::string &response);
}
