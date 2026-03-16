#include "../include/ClientHandler.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

ClientHandler::ClientHandler(int sock, CommandHandler &commandHandler)
    : client_socket(sock)
    , commandHandler(commandHandler) {}

void ClientHandler::handle() {
    std::cout << "Client connected: socket=" << client_socket << "\n";

    char tmp[4096];
    while (true) {
        ssize_t bytes = recv(client_socket, tmp, sizeof(tmp), 0);
        if (bytes <= 0) break; // 0 = disconnected, -1 = error

        parser.feed(tmp, bytes);

        std::vector<std::string> args;
        while (!(args = parser.tryParse()).empty()) {
            std::string response = commandHandler.execute(args);
            sendResponse(response);
        }
    }

    close(client_socket);
    std::cout << "Client disconnected: socket=" << client_socket << "\n";
}

void ClientHandler::sendResponse(const std::string &response) {
    size_t total = 0;
    while (total < response.size()) {
        ssize_t sent = send(client_socket, response.c_str() + total, response.size() - total, 0);
        if (sent <= 0) break;
        total += sent;
    }
}
