#ifndef C06598CE_9FFF_46D9_9E7B_B74913B603B3
#define C06598CE_9FFF_46D9_9E7B_B74913B603B3

#include <memory>
#include <string>
#include <chrono>

#include "driver.hpp"

namespace tasker {

class Job {
   protected:
    std::string job_id;
    std::string job_name;
    std::string client_id;
    std::shared_ptr<tasker::Driver> driver;
    std::vector<std::shared_ptr<std::function<void(std::string, int32_t, std::string)>>> on_complete_cbs{};
    bool completed = false;

    int32_t reported_code;
    std::string reported_msg;

    std::chrono::_V2::system_clock::time_point job_scheduled_time;
    std::chrono::_V2::system_clock::time_point worker_start_time;

    void NotifyCompletion(int32_t code = 0, std::string msg = "");

    void NotifyWorkerAllocated();

   public:
    Job(std::string job_id, std::string client_id, std::shared_ptr<tasker::Driver> driver);

    void OnComplete(std::shared_ptr<std::function<void(std::string, int32_t, std::string)>> cb);

    bool IsCompleted();

    virtual void OnWorkerMessage(std::string &worker_id, std::string &msg);

    virtual void OnWorkerRevoked(std::string &worker_id);

    void SetName(std::string name);

    std::string GetName();

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

    std::string jobs_name= "";
    std::string client_id= "";

    std::chrono::_V2::system_clock::time_point jobs_scheduled_time;

    void ReportJobCompletion(std::string job_id, int32_t code, std::string msg);

   public:
    Jobs(std::shared_ptr<std::function<void(int32_t, int32_t, std::string)>> on_all_completed,
         std::shared_ptr<std::function<void(std::string, int32_t, std::string, int32_t, int32_t)>> on_job_completed,
         std::shared_ptr<tasker::Driver> driver, std::string client_id, std::string jobs_name) : driver(driver), on_all_completed(on_all_completed), on_job_completed(on_job_completed), jobs_name(jobs_name), client_id(client_id) {
             this->jobs_scheduled_time = std::chrono::high_resolution_clock::now();
    }

    void Execute();

    void AddJob(std::shared_ptr<tasker::Job> job);

    ~Jobs() {
        spdlog::info("Deleting jobs instance...");
    }
};

}  // namespace tasker

#endif /* C06598CE_9FFF_46D9_9E7B_B74913B603B3 */
