#pragma once

#include <atomic>
#include "CommandHandler.hpp"
#include "DataBase.hpp"

class RedisServer {
public:
    RedisServer(int port);
    ~RedisServer();
    void run();
    void shutdown();

private:
    void setup();
    void acceptLoop();

    int port;
    int server_socket;
    std::atomic<bool> running;

    CommandHandler commandHandler;
    DataBase db;
};
