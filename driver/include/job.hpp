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

    void NotifyCompletion(int32_t code = 0, std::string msg = "");

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

class Jobs {
   private:
    std::vector<std::shared_ptr<tasker::Job>> jobs{};
    std::shared_ptr<std::function<void(int32_t, int32_t, std::string)>> on_all_completed;
    std::shared_ptr<std::function<void(std::string, int32_t, std::string, int32_t, int32_t)>> on_job_completed;
    std::shared_ptr<tasker::Driver> driver;

    int32_t completions = 0;
    int32_t failed_count = 0;

    // latest error codes
    int32_t latest_code = 0;
    std::string latest_msg = "";

    void ReportJobCompletion(std::string job_id, int32_t code, std::string msg);

   public:
    Jobs(std::shared_ptr<std::function<void(int32_t, int32_t, std::string)>> on_all_completed,
         std::shared_ptr<std::function<void(std::string, int32_t, std::string, int32_t, int32_t)>> on_job_completed,
         std::shared_ptr<tasker::Driver> driver) : driver(driver), on_all_completed(on_all_completed), on_job_completed(on_job_completed) {
    }

    void Execute();

    void AddJob(std::shared_ptr<tasker::Job> job);

    ~Jobs() {
        spdlog::info("Deleting jobs instance...");
    }
};

}  // namespace tasker

#endif /* C06598CE_9FFF_46D9_9E7B_B74913B603B3 */
