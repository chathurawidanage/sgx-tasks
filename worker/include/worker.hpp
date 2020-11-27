#ifndef BCEF8E54_DCF8_411B_93BD_B438AA05E830
#define BCEF8E54_DCF8_411B_93BD_B438AA05E830
#include <functional>
#include <string>
#include <zmq.hpp>

#include "messages.hpp"
#include "spdlog/spdlog.h"

namespace tasker {
class Worker {
   private:
    std::string SPACE = " ";
    zmq::socket_t *socket;
    const std::string id;
    std::function<void(std::string)> on_message;

   public:
    Worker(const std::string &id);

    void OnMessage(const std::function<void(std::string)> &on_message);

    void Send(const std::string &cmd, const std::string &msg = "") const;

    int Start(std::string &driver_address);
};
}  // namespace tasker
#endif /* BCEF8E54_DCF8_411B_93BD_B438AA05E830 */
