#include <string>

namespace tasker {
enum Commands {
    JOIN,
    EXE
};

static const std::string CommandString[] = {"JIN", "EXE"};

const std::string& GetCommand(int enumVal) {
    return CommandString[enumVal];
}

}  // namespace tasker