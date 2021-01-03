#ifndef FAD2E185_CEDB_48F5_A5D8_CECBBEC2C734
#define FAD2E185_CEDB_48F5_A5D8_CECBBEC2C734

#include "commands.hpp"
#include "driver.hpp"
#include "executor.hpp"
#include "index_job.hpp"
#include "job.hpp"
#include "job_utils.hpp"
#include "messages.hpp"
#include "uuid.hpp"
#include "worker_types.hpp"

class IndexJob : public tasker::Job {
    std::string input_file;
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;
    int32_t partition_idx;

    IndexCommand *index_command;

   public:
    IndexJob(std::string input_file,
             std::string job_id,
             int32_t partition_idx,
             std::string client_id,
             std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {
        this->input_file = input_file;
        this->partition_idx = partition_idx;

        std::string validation_msg;
        int32_t validation_code;

        std::string cmd = "idx -s " + input_file;
        this->index_command = new IndexCommand(cmd);
        this->index_command->Parse(&validation_code, &validation_msg);

        if (validation_code != 0) {
            this->NotifyCompletion(validation_code, validation_msg);
        }
    }

    void OnWorkerMessage(std::string &worker_id, std::string &rsp) {
        spdlog::info("Message from worker {}, {}", worker_id, rsp);

        int32_t error_code;
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
        spdlog::debug("After revoke func", worker_id);
    }

    bool Progress() {
        if (worker == nullptr && !this->IsCompleted()) {
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_UNSECURE);
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);
                spdlog::info("Sending command to worker {}", this->index_command->GetCommand());
                this->worker->Send(this->index_command->GetCommand());
            } else {
                spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->IsCompleted();
    }

    void Finalize() {
        spdlog::info("Finalizing index job {}", this->job_id);
        if (this->worker != nullptr) {
            driver->GetExecutor()->ReleaseWorker(*this, this->worker);
        }
    }

    ~IndexJob() {
        delete this->index_command;
    }
};
#endif /* FAD2E185_CEDB_48F5_A5D8_CECBBEC2C734 */
