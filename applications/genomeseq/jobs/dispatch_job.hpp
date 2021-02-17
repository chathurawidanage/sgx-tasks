#ifndef B87388C5_91F5_454A_B5CC_2963C628E8C9
#define B87388C5_91F5_454A_B5CC_2963C628E8C9

#include "commands.hpp"
#include "driver.hpp"
#include "executor.hpp"
#include "index_job.hpp"
#include "job.hpp"
#include "job_utils.hpp"
#include "messages.hpp"
#include "uuid.hpp"
#include "worker_types.hpp"

class DispatchJob : public tasker::Job {
   private:
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    DispatchCommand *dispatch_command;

   public:
    DispatchJob(std::string command, std::string job_id, std::string client_id,
                int partition_idx,
                std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {

        this->SetName("Dispatch-"+std::to_string(partition_idx));

        std::string validation_msg;
        int32_t validation_code;
        std::string root_dir = get_root();

        this->dispatch_command = new DispatchCommand(command);
        this->dispatch_command->Parse(&validation_code, &validation_msg);
        if (validation_code != 0) {
            // send the error to the client
            driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), validation_msg);
            this->NotifyCompletion(validation_code, validation_msg);
        } else {
            spdlog::info("Dispatch command with params bmer : {}, pnum : {}", this->dispatch_command->GetBmer(),
                         this->dispatch_command->GetPartitions());
        }
    }

    DispatchCommand *GetCommand() {
        return this->dispatch_command;
    }

    bool Progress() {
        if (worker == nullptr && !this->IsCompleted()) {
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_SECURE);
            if (this->worker != nullptr) {
                this->NotifyWorkerAllocated();
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->dispatch_command->GetCommand());
                this->worker->Send(this->dispatch_command->GetCommand());
            } else {
                spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->IsCompleted();
    }

    void OnWorkerMessage(std::string &worker_id, std::string &rsp) {
        spdlog::info("Message from worker {}, {}", worker_id, rsp);

        int32_t error_code;
        std::string cmd, msg;
        decode_response(rsp, &cmd, &error_code, &msg);

        if (error_code != 0) {
            spdlog::warn("Error reported from worker {}", error_code);

            // reporting the error to the client
            driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), msg);
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

    void Finalize() {
        spdlog::info("Finalizing job {}", this->job_id);
        if (this->worker != nullptr) {
            this->driver->GetExecutor()->ReleaseWorker(*this, this->worker);
            this->worker = nullptr;
        }
    }
};
#endif /* B87388C5_91F5_454A_B5CC_2963C628E8C9 */
