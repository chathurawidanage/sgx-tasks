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
    std::unordered_map<std::string, std::shared_ptr<Job>> jobs{};
    std::mutex jobs_lock{};

    std::set<std::string> all_workers{};
    std::queue<std::shared_ptr<tasker::WorkerHandler>> available_workers{};
    std::unordered_map<std::string, std::shared_ptr<tasker::WorkerHandler>> busy_workers{};
    std::mutex workers_lock{};

    std::unordered_map<std::string, std::string> worker_assignment{};  // <worker_id, job_id>

    std::unordered_map<std::string, int64_t> ping_times{};
    std::mutex ping_lock{};

    tasker::Driver &driver;

    // pring timeout
    int64_t ping_timeout = 60;

    void Progress();

    void IdentifyFailures();

   public:
    JobExecutor(tasker::Driver &driver);

    void AddJob(std::shared_ptr<Job> job, bool no_lock = false);

    void AddWorker(std::string &worker_id, std::string &worker_type);

    void OnPing(std::string &from_worker);

    bool HasWorker(std::string &worker_id);

    /**
     * This function will be called by Job to get a worker allocated for the job
     **/
    std::shared_ptr<tasker::WorkerHandler> AllocateWorker(Job &to, std::string worker_type);

    void ReleaseWorker(Job &of, std::shared_ptr<tasker::WorkerHandler> worker);

    void ForwardMsgToJob(std::string &from, std::string &msg);

    void Start();
};
}  // namespace tasker
#endif /* C9841B2A_9FFB_49B0_A051_9D2CA20A8674 */
