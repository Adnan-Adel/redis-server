#pragma once

#include <atomic>
#include "CommandHandler.hpp"
#include "DataBase.hpp"
#include "ThreadPool.hpp"

class RedisServer {
public:
    RedisServer(int port);
    ~RedisServer();
    void run();
    void shutdown();
    void dumpDatabase();

private:
    void setup();
    void acceptLoop();
    void setupSignalHandler();

    int port;
    int server_socket;
    std::atomic<bool> running;

    DataBase db;
    CommandHandler commandHandler;
    ThreadPool threadPool;
};
