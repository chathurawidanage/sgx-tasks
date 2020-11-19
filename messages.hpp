#include <string>

namespace tasker {
enum Commands {
    JOIN,
    EXE,
    MESSAGE,
    ACK
};

static const std::string CommandString[] = {"JIN", "EXE", "MSG", "ACK"};

const std::string &GetCommand(int enumVal) {
    return CommandString[enumVal];
}

}  // namespace tasker