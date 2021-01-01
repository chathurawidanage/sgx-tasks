#ifndef C06598CE_9FFF_46D9_9E7B_B74913B603B3
#define C06598CE_9FFF_46D9_9E7B_B74913B603B3

#include <memory>
#include <string>

#include "driver.hpp"

namespace tasker {

class Job {
   protected:
    std::string job_id;
    std::string client_id;
    std::shared_ptr<tasker::Driver> driver;
    std::vector<std::shared_ptr<std::function<void(std::string, int32_t, std::string)>>> on_complete_cbs{};
    bool completed = false;

    void NotifyCompletion(int32_t code, std::string msg);

   public:
    Job(std::string job_id, std::string client_id, std::shared_ptr<tasker::Driver> driver);

    void OnComplete(std::shared_ptr<std::function<void(std::string, int32_t, std::string)>> cb);

    bool IsCompleted();

    virtual void OnWorkerMessage(std::string &worker_id, std::string &msg);

    virtual void OnWorkerRevoked(std::string &worker_id);

    std::string &GetId();

    /**
     * Returns true if job is completed
     * */
    virtual bool Progress();

    virtual void Finalize();
};
}  // namespace tasker

#endif /* C06598CE_9FFF_46D9_9E7B_B74913B603B3 */
