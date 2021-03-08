#ifndef F43ED8E9_E20E_4889_8BF6_2F0E572ECC8A
#define F43ED8E9_E20E_4889_8BF6_2F0E572ECC8A

#include "commands.hpp"
#include "driver.hpp"
#include "executor.hpp"
#include "index_job.hpp"
#include "job.hpp"
#include "job_utils.hpp"
#include "messages.hpp"
#include "uuid.hpp"
#include "worker_types.hpp"

/**
 * This job will require only one worker
 * */
class MergeJob : public tasker::Job {
  std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

  MergeCommand *merge_command;  // todo delete

 public:
  MergeJob(std::string result_dir, int32_t partitions, std::string job_id, std::string client_id,
           std::shared_ptr<tasker::Driver> driver)
      : Job(job_id, client_id, driver) {
    this->SetName("Merge-" + result_dir);

    std::string validation_msg;
    int32_t validation_code;

    std::string p_cmd = "mrg -r " + get_root() + result_dir + " -p " + std::to_string(partitions) +
                        " -a bwa-mem -m ord";

    spdlog::info("Generated merge command : {}", p_cmd);

    this->merge_command = new MergeCommand(p_cmd);
    this->merge_command->Parse(&validation_code, &validation_msg);

    if (validation_code != 0) {
      this->NotifyCompletion(validation_code, validation_msg);
    }
  }

  MergeCommand *GetCommand() { return this->merge_command; }

  void OnWorkerMessage(std::string &worker_id, std::string &rsp) {
    spdlog::info("Message from worker {}, {}", worker_id, rsp);

    int32_t error_code = 0;
    std::string cmd, msg;
    decode_response(rsp, &cmd, &error_code, &msg);

    if (error_code != 0) {
      spdlog::warn("Error reported from worker {}", error_code);
    }

    this->NotifyCompletion(error_code, msg);
  }

  void OnWorkerRevoked(std::string &worker_id) {
    spdlog::info("Job notified about worker {} disconnection.", worker_id);
    if (!this->IsCompleted()) {
      this->worker = nullptr;
    }
    spdlog::info("After revoke func", worker_id);
  }

  bool Progress() {
    if (worker == nullptr && !this->IsCompleted()) {
      this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_SECURE);
      if (this->worker != nullptr) {
        this->NotifyWorkerAllocated();
        spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

        spdlog::info("Sending command to worker {}", this->merge_command->GetCommand());
        this->worker->Send(this->merge_command->GetCommand());
      } else {
        spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
      }
    }
    return this->IsCompleted();
  }

  void Finalize() {
    spdlog::info("Finalizing job {}", this->job_id);
    if (this->worker != nullptr) {
      this->driver->GetExecutor()->ReleaseWorker(*this, this->worker);
      this->worker = nullptr;
    }
  }

  ~MergeJob() {
    delete this->merge_command;
    spdlog::info("Deleting merge command...");
  }
};
#endif /* F43ED8E9_E20E_4889_8BF6_2F0E572ECC8A */
