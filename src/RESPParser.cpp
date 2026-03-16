#include "../include/RESPParser.hpp"
#include <sstream>

void RESPParser::feed(const char *data, size_t len) {
    buffer.append(data, len);
}

// std::vector<std::string> RESP::parse(const std::string &input) {
//     std::vector<std::string> tokens;
//     if (input.empty()) return tokens;
//
//     size_t pos = 0;
//     if (input[pos] != '*') {
//         std::istringstream iss(input);
//         std::string token;
//         while (iss >> token)
//             tokens.push_back(token);
//
//         return tokens;
//     }
//
//     pos++; // skip '*'
//
//     size_t crlf = input.find("\r\n", pos);
//     if (crlf == std::string::npos) return tokens;
//
//     int numElements = std::stoi(input.substr(pos, crlf - pos));
//     pos = crlf + 2;
//
//     for (int i = 0; i < numElements; i++) {
//         if (pos >= input.size() || input[pos] != '$') break;
//         pos++;
//
//         crlf = input.find("\r\n", pos);
//         if (crlf == std::string::npos) break;
//         int len = std::stoi(input.substr(pos, crlf - pos));
//         pos = crlf + 2;
//
//         if (pos + len > input.size()) break;
//         std::string token = input.substr(pos, len);
//         tokens.push_back(token);
//         pos += len + 2;
//     }
//     return tokens;
// }

std::vector<std::string> RESPParser::tryParse() {
    std::vector<std::string> tokens;
    if (buffer.empty()) return tokens;

    // inline fallback for telnet/manual testing
    if (buffer[0] != '*') {
        size_t end = buffer.find('\n');
        if (end == std::string::npos) return tokens;
        std::istringstream iss(buffer.substr(0, end));
        std::string token;
        while (iss >> token) tokens.push_back(token);
        buffer.erase(0, end + 1); // consume the line
        return tokens;
    }

    size_t pos = 1; // skip '*'
    size_t crlf = buffer.find("\r\n", pos);
    if (crlf == std::string::npos) return tokens; // incomplete

    int numElements = std::stoi(buffer.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; i++) {
        if (pos >= buffer.size() || buffer[pos] != '$') return tokens;
        pos++;

        crlf = buffer.find("\r\n", pos);
        if (crlf == std::string::npos) return tokens; // incomplete

        int len = std::stoi(buffer.substr(pos, crlf - pos));
        pos = crlf + 2;

        if (pos + len + 2 > buffer.size()) return tokens; // incomplete
        tokens.push_back(buffer.substr(pos, len));
        pos += len + 2;
    }

    buffer.erase(0, pos); // consume exactly what we parsed
    return tokens;
}

// encoding
std::string RESPParser::encodeSimple(const std::string &str) {
    return "+" + str + "\r\n";
}

std::string RESPParser::encodeError(const std::string &err) {
    return "-" + err + "\r\n";
}

std::string RESPParser::encodeBulk(const std::string &str) {
    return "$" + std::to_string(str.size()) + "\r\n" + str + "\r\n";
}

std::string RESPParser::encodeNull() {
    return "$-1\r\n";
}

std::string RESPParser::encodeInteger(int n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string RESPParser::encodeArray(const std::vector<std::string> &items) {
    std::string out = "*" + std::to_string(items.size()) + "\r\n";
    for (const auto &item : items)
        out += encodeBulk(item);
    return out;
}
