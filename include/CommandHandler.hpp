#pragma once

#include <string>
#include <vector>
#include "DataBase.hpp"

class CommandHandler {
public:
    CommandHandler(DataBase &db);

    std::string execute(const std::vector<std::string> &args);

private:
    DataBase &db;
};
