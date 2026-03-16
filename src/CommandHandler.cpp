#include "../include/CommandHandler.hpp"
#include <algorithm>
#include <sstream>

CommandHandler::CommandHandler(DataBase &db)
    : db(db) {}

std::string CommandHandler::execute(const std::vector<std::string> &args) {
    if (args.empty())
        return "-ERR empty command\r\n";

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;

    // connect to db

    // check commands
    if (cmd == "PING") {
        response << "+PONG\r\n";
    } else if (cmd == "ECHO") {

    } else {
        response << "-Error: Unknown command\r\n";
    }

    return response.str();
}
