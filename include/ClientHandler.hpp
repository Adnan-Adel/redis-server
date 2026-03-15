#pragma once

#include <string>
#include "CommandHandler.hpp"

class ClientHandler {
public:
    ClientHandler(int fd, CommandHandler &commandHandler);
    void handle();

private:
    std::string readLine();
    void sendResponse(const std::string &response);

    int client_fd;
    CommandHandler &commandHandler;
};
