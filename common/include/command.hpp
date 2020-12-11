#ifndef B5BA4448_614A_4BC7_BEFE_271324201899
#define B5BA4448_614A_4BC7_BEFE_271324201899

#include <string>

namespace tasker {
class Command {
   protected:
    std::string& command;

   public:
    Command(std::string& command) : command(command) {
    }

    virtual void parse() = 0;
    virtual void validate(int32_t* code, std::string* msg) = 0;
};
};     // namespace tasker
#endif /* B5BA4448_614A_4BC7_BEFE_271324201899 */
