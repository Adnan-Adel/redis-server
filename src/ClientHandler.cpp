#include "../include/ClientHandler.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "../include/RESPParser.hpp"
using namespace RESP;

ClientHandler::ClientHandler(int fd, CommandHandler &commandHandler)
    : client_fd(fd), commandHandler(commandHandler) {}

void ClientHandler::handle() {
    std::cout << "Client connected: fd=" << client_fd << "\n";

    while (true) {
        std::string line = readLine();
        if (line.empty()) break;

        std::vector<std::string> args = RESP::parse(line);
        std::string response = commandHandler.execute(args);
        sendResponse(response);
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

void ClientHandler::sendResponse(const std::string &response) {
    send(client_fd, response.c_str(), response.size(), 0);
}
