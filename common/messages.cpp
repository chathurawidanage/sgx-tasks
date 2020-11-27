#include "include/messages.hpp"

const std::string &tasker::GetCommand(int enumVal) {
    return CommandString[enumVal];
}