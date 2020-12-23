#ifndef F7631D67_A986_450C_A1AE_288C1613EBF9
#define F7631D67_A986_450C_A1AE_288C1613EBF9
#include <string>

#include "driver.hpp"
#include "spdlog/spdlog.h"

namespace tasker {
class WorkerHandler {
   private:
    std::string worker_id;
    std::string worker_type;
    tasker::Driver &driver;

   public:
    WorkerHandler(std::string worker_id,
                  std::string worker_type,
                  tasker::Driver &driver) : worker_id(worker_id), worker_type(worker_type), driver(driver) {
    }

    std::string &GetId() {
        return this->worker_id;
    }

    void Send(std::string &msg) {
        spdlog::info("Sending a message {} to {}", msg, this->worker_id);
        std::string msg_with_prefix;
        msg_with_prefix.reserve(4 + msg.size());
        msg_with_prefix.append("MSG ");
        msg_with_prefix.append(msg);
        this->driver.SendToWorker(this->worker_id, msg_with_prefix);
    }
};
}  // namespace tasker
#endif /* F7631D67_A986_450C_A1AE_288C1613EBF9 */
