#include "../include/RedisServer.hpp"
#include "../include/ClientHandler.hpp"
#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>

static RedisServer *globalServer = nullptr;

void signalHandler(int signum) {
    if (globalServer) {
        std::cout << "Caught signal " << signum << ", shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
}

RedisServer::RedisServer(int port)
    : port(port)
    , server_socket(-1)
    , running(false)
    , commandHandler(db)
    , threadPool(std::thread::hardware_concurrency()) {
    globalServer = this;
    setupSignalHandler();
}

RedisServer::~RedisServer() {
    if (server_socket != -1)
        close(server_socket);
}

void RedisServer::setup() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        throw std::runtime_error("Failed to create socket.");
    }
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options.");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind on port " + std::to_string(port) + " — already in use.");
    }

    if (listen(server_socket, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket.");
    }

    std::cout << "Redis server listening on port " << port << "\n";
}

void RedisServer::acceptLoop() {
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int client_socket = accept(server_socket, (struct sockaddr *)&clientAddr, &clientLen);
        if (client_socket < 0) {
            if (!running) break;
            std::cerr << "Failed to accept connection.\n";
            continue;
        }

        threadPool.enqueue([this, client_socket]() {
            ClientHandler handler(client_socket, commandHandler);
            handler.handle();
        });
    }
}

void RedisServer::dumpDatabase() {
    if (db.dump("dumped.db"))
        std::cout << "Database dumped.\n";
    else
        std::cerr << "Error dumping database.\n";
}

void RedisServer::run() {
    try {
        setup();
    } catch (const std::exception &e) {
        std::cerr << "Server failed to start: " << e.what() << "\n";
        return;
    }
    running = true;
    acceptLoop();

    // persist db
    dumpDatabase();
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    std::cout << "Server shutdown successfully.\n";
}
