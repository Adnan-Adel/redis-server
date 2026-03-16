#pragma once

#include <string>
#include "CommandHandler.hpp"

class ClientHandler {
public:
    ClientHandler(int fd, CommandHandler &commandHandler);
    void handle();

private:
    std::string readLine();
    bool readIntoBuffer();
    void sendResponse(const std::string &response);

    int client_fd;
    CommandHandler &commandHandler;
    std::string buffer;
};
