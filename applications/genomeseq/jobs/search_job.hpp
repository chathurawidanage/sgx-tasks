#ifndef A7483C2B_7054_48CD_B89A_4CFA103028FE
#define A7483C2B_7054_48CD_B89A_4CFA103028FE

#include "commands.hpp"
#include "driver.hpp"
#include "executor.hpp"
#include "index_job.hpp"
#include "job.hpp"
#include "job_utils.hpp"
#include "messages.hpp"
#include "uuid.hpp"
#include "worker_types.hpp"

class SearchJob : public tasker::Job {
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    SearchCommand *search_command;

   public:
    SearchJob(std::string src_file,
              std::string index_file,
              std::string dst_file,
              std::string job_id,
              std::string client_id,
              std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {
        std::string validation_msg;
        int32_t validation_code;

        std::string cmd = "mem -s " + src_file + " -i " + index_file + " -d " + dst_file;
        this->search_command = new SearchCommand(cmd);
        this->search_command->Parse(&validation_code, &validation_msg);

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
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_SECURE_GRAPHENE);
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);
                spdlog::info("Sending command to worker {}", this->search_command->GetCommand());
                this->worker->Send(this->search_command->GetCommand());
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

    ~SearchJob() {
        delete this->search_command;
    }
};
#endif /* A7483C2B_7054_48CD_B89A_4CFA103028FE */
