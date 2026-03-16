#include "../include/RedisServer.hpp"
#include <thread>
#include <chrono>

int main(int argc, char *argv[]) {
    int port = 6379;
    if (argc >= 2)
        port = std::stoi(argv[1]);

    RedisServer server(port);

    // Background persistence: dump database every 300 seconds.
    std::thread persistanceThread([&server]() {
        std::this_thread::sleep_for(std::chrono::seconds(300));
        // dump database
        server.dumpDatabase();
    });
    persistanceThread.detach();

    server.run();

    return 0;
}
