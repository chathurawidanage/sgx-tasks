#include <string>

namespace tasker {
enum Commands {
    JOIN,
    MESSAGE,
    ACK
};

static const std::string CommandString[] = {"JIN", "MSG", "ACK"};

const std::string &GetCommand(int enumVal) {
    return CommandString[enumVal];
}

}  // namespace tasker