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
    const std::string type;
    std::function<void(std::string)> on_message;

    int64_t ping_interval = 10000;

   public:
    Worker(const std::string &id,const std::string &type);

    void OnMessage(const std::function<void(std::string)> &on_message);

    void Send(const std::string &cmd, const std::string &msg = "") const;

    int Start(std::string &driver_address);

    const std::string &GetId();

    int64_t GetPingInterval();
};
}  // namespace tasker
#endif /* BCEF8E54_DCF8_411B_93BD_B438AA05E830 */
