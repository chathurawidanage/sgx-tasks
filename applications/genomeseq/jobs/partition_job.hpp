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
    bool job_done = false;

    int32_t error_code = 0;

   public:
    PartitionJob(std::string command, std::string job_id, std::string client_id,
                 std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {
        std::string validation_msg;
        int32_t validation_code;

        // client sends a index command, but we have to partition first
        auto temp_index_cmd = ClientIndexCommand(command);
        temp_index_cmd.Parse(&validation_code, &validation_msg);
        if (validation_code != 0) {
            // send the error to the client
            driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), validation_msg);
            this->job_done = true;
        } else {
            std::string input_file_name = std::filesystem::path(temp_index_cmd.GetSrcFile()).filename();

            // std::replace(input_file_name.begin(), input_file_name.end(), '.', '_');
            // create a random index id
            this->index_id = "index-" + gen_random(8);
            std::string p_cmd = "prt -s " + temp_index_cmd.GetRelativeSrcFile() + " -p " + std::to_string(temp_index_cmd.GetPartitions()) + " -d " + index_id;

            spdlog::info("Generated partition command : {}", p_cmd);

            this->partition_command = new PartitionCommand(p_cmd);
            this->partition_command->Parse(&validation_code, &validation_msg);

            if (validation_code != 0) {
                driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), validation_msg);
                this->job_done = true;
            }
        }
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

    bool Progress() {
        if (worker == nullptr && !this->job_done) {
            this->worker = driver->GetExecutor()->AllocateWorker(*this, TYPE_UNSECURE);
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->partition_command->GetCommand());
                this->worker->Send(this->partition_command->GetCommand());
            } else {
                spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->job_done;
    }

    void Finalize() {
        spdlog::info("Finalizing job {}", this->job_id);
        if (this->worker != nullptr) {
            this->driver->GetExecutor()->ReleaseWorker(*this, this->worker);
        }

        // now schedule index jobs
        if (this->error_code == 0) {
            auto index_counter = std::make_shared<int32_t>(0);

            auto total_jobs = std::make_shared<int32_t>(this->partition_command->GetPartitions());

            auto faile_job = std::make_shared<bool>(false);

            auto driver_ptr = this->driver;

            auto clinet_id_ptr = std::make_shared<std::string>(client_id);
            auto index_id_ptr = std::make_shared<std::string>(index_id);
            auto src_file_ptr = std::make_shared<std::string>(this->partition_command->GetSrcFile());

            spdlog::info("Creating indexing jobs for {} partitions.", this->partition_command->GetPartitions());
            for (int32_t idx = 0; idx < this->partition_command->GetPartitions(); idx++) {
                std::string idx_src = this->index_id + "/mref-" + std::to_string(idx + 1) + ".fa";

                std::string job_id = gen_random(16);
                std::shared_ptr<IndexJob> idx_job = std::make_shared<IndexJob>(
                    idx_src, job_id,
                    idx,
                    std::make_shared<std::function<void(int32_t, int32_t, std::string)>>([index_counter, driver_ptr, clinet_id_ptr, faile_job, total_jobs, index_id_ptr, src_file_ptr](int32_t idx, int32_t error_code, std::string msg) {
                        spdlog::info("Indexing done for {} {} {}", idx, error_code, msg);
                        spdlog::info("Own count {}", index_counter.use_count());
                        (*index_counter)++;
                        if (error_code == 0) {
                            spdlog::info("Indexing completed for {}", *index_counter);
                        } else {
                            spdlog::warn("Failed indexing partition {} {}", idx, *index_counter);
                            // one has failed indexing
                            if (!(*faile_job)) {
                                std::string error_msg = "Indexing failed for partition " + std::to_string(idx) + ". " + msg;
                                driver_ptr->SendToClient(*clinet_id_ptr, tasker::GetCommand(tasker::Commands::MESSAGE), error_msg);
                            }

                            // TODO remove other jobs to prevent wasting resources
                            *faile_job = true;
                        }

                        spdlog::info("IF COMP {} {}", *index_counter, *total_jobs);
                        if (*index_counter == *total_jobs) {
                            spdlog::info("All indexing jobs have been completed.");
                            if (!(*faile_job)) {
                                driver_ptr->SendToClient(*clinet_id_ptr, tasker::GetCommand(tasker::Commands::MESSAGE),
                                                         "Indexing completed and assigned ID " + (*index_id_ptr));

                                // adding index to the DB
                                // indices_lock.lock();
                                // indices.push_back(std::make_shared<Index>(*total_jobs, *index_id_ptr, *src_file_ptr));
                                // indices_lock.unlock();
                            } else {
                                spdlog::warn("At least one of the indexing jobs has failed...");
                                // TODO cleanup out directories
                            }
                        }
                    }),
                    client_id,
                    driver);
                spdlog::info("Created index job for partition {}", idx + 1);
                driver->GetExecutor()->AddJob(idx_job);
                spdlog::info("Added job for partition {}", idx + 1);
            }
        } else {
            spdlog::info("Partition job has failed. Not scheduling index jobs");
        }
    }

    ~PartitionJob() {
        delete this->partition_command;
        spdlog::info("Deleting partition command...");
    }
};
#endif /* F43ED8E9_E20E_4889_8BF6_2F0E572ECC8B */
