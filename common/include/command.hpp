#ifndef B5BA4448_614A_4BC7_BEFE_271324201899
#define B5BA4448_614A_4BC7_BEFE_271324201899

#include <string>

namespace tasker {
class Command {
   protected:
    std::string command;

    virtual void Validate(int32_t* code, std::string* msg) = 0;

   public:
    Command(std::string command) : command(command) {
    }

    std::string& GetCommand() {
        return this->command;
    }

    virtual void Parse(int32_t* code, std::string* msg) = 0;
};
};     // namespace tasker
#endif /* B5BA4448_614A_4BC7_BEFE_271324201899 */
