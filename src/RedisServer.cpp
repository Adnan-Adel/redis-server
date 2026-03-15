#include "../include/RedisServer.hpp"
#include "../include/ClientHandler.hpp"
#include <csignal>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>

static RedisServer *globalServer = nullptr;

static void handleSignal(int) {
    if (globalServer)
        globalServer->shutdown();
}

RedisServer::RedisServer(int port)
    : port(port), server_socket(-1), running(false), commandHandler(db) {
    globalServer = this;
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
}

RedisServer::~RedisServer() {
    if (server_socket != -1)
        close(server_socket);
}

void RedisServer::setup() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket.\n";
        return;
    }
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket option.\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket.\n";
        return;
    }

    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket.\n";
        return;
    }

    std::cout << "Redis server listening on port " << port << "\n";
}

void RedisServer::acceptLoop() {
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int client_fd = accept(server_socket, (struct sockaddr *)&clientAddr, &clientLen);
        if (client_fd < 0) {
            if (!running) break;
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        // detach so we don't have to join — client owns its lifetime
        std::thread([this, client_fd]() {
            ClientHandler handler(client_fd, commandHandler);
            handler.handle();
        }).detach();
    }
}

void RedisServer::run() {
    setup();
    running = true;
    acceptLoop();
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    std::cout << "Server shutdown successfully.\n";
}
