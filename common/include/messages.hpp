#ifndef AD143021_2B48_4096_A1D5_EEEB1CFD06E9
#define AD143021_2B48_4096_A1D5_EEEB1CFD06E9
#include <string>

namespace tasker {
enum Commands {
    JOIN,
    MESSAGE,
    ACK,
    PING
};

static const std::string CommandString[] = {"JIN", "MSG", "ACK", "PNG"};

const std::string &GetCommand(int enumVal);

}  // namespace tasker
#endif /* AD143021_2B48_4096_A1D5_EEEB1CFD06E9 */
