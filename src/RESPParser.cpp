#include "../include/RESPParser.hpp"
#include <sstream>

std::vector<std::string> RESP::parse(const std::string &input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    size_t pos = 0;
    if (input[pos] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token)
            tokens.push_back(token);

        return tokens;
    }

    pos++; // skip '*'

    size_t crlf = input.find("\r\n", pos);
    if (crlf == std::string::npos) return tokens;

    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; i++) {
        if (pos >= input.size() || input[pos] != '$') break;
        pos++;

        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        int len = std::stoi(input.substr(pos, crlf - pos));
        pos = crlf + 2;

        if (pos + len > input.size()) break;
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len + 2;
    }
    return tokens;
}

std::pair<std::vector<std::string>, size_t> RESP::tryParse(const std::string &buf) {
    std::vector<std::string> tokens;
    if (buf.empty()) return { tokens, 0 };

    // inline fallback for manual testing via telnet
    if (buf[0] != '*') {
        size_t end = buf.find('\n');
        if (end == std::string::npos) return { tokens, 0 }; // incomplete
        std::istringstream iss(buf.substr(0, end));
        std::string token;
        while (iss >> token) tokens.push_back(token);
        return { tokens, end + 1 };
    }

    size_t pos = 1; // skip '*'
    size_t crlf = buf.find("\r\n", pos);
    if (crlf == std::string::npos) return { tokens, 0 }; // incomplete

    int numElements = std::stoi(buf.substr(pos, crlf - pos));
    pos = crlf + 2;

    for (int i = 0; i < numElements; i++) {
        if (pos >= buf.size() || buf[pos] != '$') return { tokens, 0 };
        pos++;

        crlf = buf.find("\r\n", pos);
        if (crlf == std::string::npos) return { tokens, 0 }; // incomplete

        int len = std::stoi(buf.substr(pos, crlf - pos));
        pos = crlf + 2;

        if (pos + len + 2 > buf.size()) return { tokens, 0 }; // incomplete
        tokens.push_back(buf.substr(pos, len));
        pos += len + 2;
    }

    return { tokens, pos }; // pos is exactly how many bytes we consumed
}
