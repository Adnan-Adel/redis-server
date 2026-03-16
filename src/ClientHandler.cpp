#include "../include/ClientHandler.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "../include/RESPParser.hpp"

ClientHandler::ClientHandler(int fd, CommandHandler &commandHandler)
    : client_fd(fd)
    , commandHandler(commandHandler) {}

void ClientHandler::handle() {
    std::cout << "Client connected: fd=" << client_fd << "\n";

    while (true) {
        if (!readIntoBuffer()) break;

        while (true) {
            auto [args, consumed] = RESP::tryParse(buffer);
            if (consumed == 0) break;

            buffer.erase(0, consumed);

            if (args.empty()) continue;
            std::string response = commandHandler.execute(args);
            sendResponse(response);
        }
    }

    close(client_fd);
    std::cout << "Client disconnected: fd=" << client_fd << "\n";
}

std::string ClientHandler::readLine() {
    std::string result;
    char ch;
    while (recv(client_fd, &ch, 1, 0) > 0) {
        if (ch == '\n') break;
        if (ch != '\r') result += ch;
    }
    return result;
}

bool ClientHandler::readIntoBuffer() {
    char tmp[4096];
    ssize_t bytes = recv(client_fd, tmp, sizeof(tmp), 0);
    if (bytes <= 0) return false;
    buffer.append(tmp, bytes);
    return true;
}

void ClientHandler::sendResponse(const std::string &response) {
    size_t total = 0;
    while (total < response.size()) {
        ssize_t sent = send(client_fd, response.c_str() + total, response.size() - total, 0);
        if (sent <= 0) break;
        total += sent;
    }
}
