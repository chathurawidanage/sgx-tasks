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
    bool job_done = false;
    int32_t error_code = 0;

    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    DispatchCommand *dispatch_command;

   public:
    DispatchJob(std::string command, std::string job_id, std::string client_id,
                std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {
        std::string validation_msg;
        int32_t validation_code;
        std::string root_dir = get_root();

        std::string result_id = gen_random(8);

        command = command + " -d result-" + result_id;

        spdlog::info("Modified dsp command with destination {}", command);

        this->dispatch_command = new DispatchCommand(command);
        this->dispatch_command->Parse(&validation_code, &validation_msg);
        if (validation_code != 0) {
            // send the error to the client
            driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), validation_msg);
            this->job_done = true;
        } else {
            spdlog::info("Dispatch command with params bmer : {}, pnum : {}", this->dispatch_command->GetBmer(),
                         this->dispatch_command->GetPartitions());
        }
    }

    bool Progress() {
        if (worker == nullptr && !this->job_done) {
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_SECURE);
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->dispatch_command->GetCommand());
                this->worker->Send(this->dispatch_command->GetCommand());
            } else {
                spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->job_done;
    }

    void OnWorkerMessage(std::string &worker_id, std::string &rsp) {
        spdlog::info("Message from worker {}, {}", worker_id, rsp);

        std::string cmd, msg;
        decode_response(rsp, &cmd, &this->error_code, &msg);

        if (error_code != 0) {
            spdlog::warn("Error reported from worker {}", error_code);

            // reporting the error to the client
            driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), msg);
        }

        this->job_done = true;
    }

    void OnWorkerRevoked(std::string &worker_id) {
        spdlog::info("Job notified about worker {} disconnection.", worker_id);
        if (!this->job_done) {
            this->worker = nullptr;
        }
        spdlog::info("After revoke func", worker_id);
    }
};
#endif /* B87388C5_91F5_454A_B5CC_2963C628E8C9 */
