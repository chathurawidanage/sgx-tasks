#ifndef F7631D67_A986_450C_A1AE_288C1613EBF9
#define F7631D67_A986_450C_A1AE_288C1613EBF9
#include <string>

#include "driver.hpp"
namespace tasker {
class Worker {
   private:
    std::string &worker_id;
    std::string &worker_type;
    tasker::Driver &driver;

   public:
    Worker(std::string &worker_id,
           std::string &worker_type,
           tasker::Driver &driver) : worker_id(worker_id), worker_type(worker_type), driver(driver) {
    }

    void Send(std::string &msg) {
        this->driver.SendToWorker(this->worker_id, msg);
    }
};
}  // namespace tasker
#endif /* F7631D67_A986_450C_A1AE_288C1613EBF9 */
