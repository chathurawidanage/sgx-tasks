#ifndef F43ED8E9_E20E_4889_8BF6_2F0E572ECC8B
#define F43ED8E9_E20E_4889_8BF6_2F0E572ECC8B

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
class PartitionJob : public tasker::Job {
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    PartitionCommand *partition_command;  //todo delete

    std::string index_id;

   public:
    PartitionJob(std::string command, std::string job_id, std::string client_id,
                 std::string index_id,
                 std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver), index_id(index_id) {
        std::string validation_msg;
        int32_t validation_code;

        // client sends a index command, but we have to partition first
        auto temp_index_cmd = ClientIndexCommand(command);
        temp_index_cmd.Parse(&validation_code, &validation_msg);
        if (validation_code != 0) {
            this->NotifyCompletion(validation_code, validation_msg);
        } else {
            std::string input_file_name = std::filesystem::path(temp_index_cmd.GetSrcFile()).filename();

            std::string p_cmd = "prt -s " + temp_index_cmd.GetRelativeSrcFile() + " -p " + std::to_string(temp_index_cmd.GetPartitions()) + " -d " + index_id;

            spdlog::info("Generated partition command : {}", p_cmd);

            this->partition_command = new PartitionCommand(p_cmd);
            this->partition_command->Parse(&validation_code, &validation_msg);

            if (validation_code != 0) {
                this->NotifyCompletion(validation_code, validation_msg);
            }
        }
    }

    PartitionCommand *GetCommand() {
        return this->partition_command;
    }

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
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_UNSECURE);
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->partition_command->GetCommand());
                this->worker->Send(this->partition_command->GetCommand());
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
        }
    }

    ~PartitionJob() {
        delete this->partition_command;
        spdlog::info("Deleting partition command...");
    }
};
#endif /* F43ED8E9_E20E_4889_8BF6_2F0E572ECC8B */
