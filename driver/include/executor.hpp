#ifndef C9841B2A_9FFB_49B0_A051_9D2CA20A8674
#define C9841B2A_9FFB_49B0_A051_9D2CA20A8674
#include <list>
#include <mutex>
#include <queue>

#include "job.hpp"
#include "worker_handler.hpp"

namespace tasker {

class Driver;

class JobExecutor {
   private:
    std::list<Job> jobs{};
    std::mutex lock{};
    std::queue<std::shared_ptr<tasker::WorkerHandler>> available_workers{};

    tasker::Driver &driver;

    void Progress();

   public:
    JobExecutor(tasker::Driver &driver);

    void AddJob(Job &job);

    void AddWorker(std::string &worker_id, std::string &worker_type) {
        this->available_workers.push(std::make_shared<tasker::WorkerHandler>(worker_id, worker_type, driver));
    }

    /**
     * This function will be called by Job to get a worker allocated for the job
     **/
    std::shared_ptr<tasker::WorkerHandler> AllocateWorker(Job &to, std::string worker_type);

    void ReleaseWorker(std::shared_ptr<tasker::WorkerHandler> worker);

    void Start();
};
}  // namespace tasker
#endif /* C9841B2A_9FFB_49B0_A051_9D2CA20A8674 */
