#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include "commands.hpp"
#include "driver.hpp"
#include "executor.hpp"
#include "job.hpp"
#include "messages.hpp"
#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker_handler.hpp"
#include "worker_types.hpp"

std::string root_dir = get_root();

class Index {
   private:
    int32_t partitions;
    std::string id;
    std::string source_file;

   public:
    Index(int32_t partitions, std::string id, std::string source_file) : partitions(partitions), id(id), source_file(source_file) {
    }

    std::string Print() {
        std::stringstream ss;

        ss << id << "\t\t" << source_file << "\t\t" << partitions;

        return ss.str();
    }
};

std::mutex indices_lock;
std::vector<std::shared_ptr<Index>> indices{};

void decode_response(std::string &rsp, std::string *cmd, int32_t *error_code, std::string *msg) {
    *cmd = rsp.substr(0, 3);
    std::string err_code = rsp.substr(4, rsp.find(' ', 5) - 4);
    *error_code = std::stoi(err_code);

    int32_t msg_start = 5 + err_code.size();
    if (msg_start < rsp.size()) {
        *msg = rsp.substr(msg_start, rsp.size());
    } else {
        *msg = "";
        if (*error_code != 0) {
            *msg = "Unknown error occurred. Error code " + err_code;
        } else {
            *msg = "";
        }
    }
}

class IndexJob : public tasker::Job {
    std::string input_file;
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;
    bool job_done = false;
    int32_t partition_idx;
    std::shared_ptr<std::function<void(int32_t, int32_t, std::string)>> on_complete;

    IndexCommand *index_command;  //todo delete

   public:
    IndexJob(std::string input_file,
             std::string job_id,
             int32_t partition_idx,
             std::shared_ptr<std::function<void(int32_t, int32_t, std::string)>> on_complete,
             std::string client_id,
             std::shared_ptr<tasker::Driver> driver) : Job(job_id, client_id, driver) {
        this->input_file = input_file;
        this->partition_idx = partition_idx;
        this->on_complete = on_complete;

        std::string validation_msg;
        int32_t validation_code;

        std::string cmd = "idx -s " + input_file;
        this->index_command = new IndexCommand(cmd);
        this->index_command->Parse(&validation_code, &validation_msg);

        if (validation_code != 0) {
            spdlog::info("Calling on complete...");
            (*(this->on_complete))(this->partition_idx, validation_code, validation_msg);
            spdlog::info("Called on complete...");
            this->job_done = true;
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

        this->job_done = true;

        // calling the callback
        (*(this->on_complete))(this->partition_idx, error_code, msg);
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
                spdlog::info("Sending command to worker {}", this->index_command->GetCommand());
                this->worker->Send(this->index_command->GetCommand());
            } else {
                spdlog::debug("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->job_done;
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

/**
 * This job will require only one worker
 * */
class PartitionJob : public tasker::Job {
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    PartitionCommand *partition_command;  //todo delete

    std::string index_id = "";
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
                                indices_lock.lock();
                                indices.push_back(std::make_shared<Index>(*total_jobs, *index_id_ptr, *src_file_ptr));
                                indices_lock.unlock();
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

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    // spdlog::set_pattern("[%H:%M:%S %z] [%l] [trd %t] %v");

    std::shared_ptr<tasker::Driver> driver = std::make_shared<tasker::Driver>();

    driver->SetOnWorkerJoined([&driver](std::string &worker_id, std::string &worker_type) {
        spdlog::info("Worker joined : {}", worker_id);
    });

    driver->SetOnClientConnected([](std::string &client_id, std::string &client_meta) {
        spdlog::info("Client connected : {}", client_id);
    });

    driver->SetOnWorkerMsg([&driver](std::string &worker_id, std::string &msg) {
        spdlog::info("Worker message from {} : {}", worker_id, msg);
    });

    driver->SetOnClientMsg([driver](std::string &client_id, std::string &msg) {
        spdlog::info("Clinet message from [{}] : {}", client_id, msg);
        // extracting out task
        std::istringstream stream(msg);
        std::string task_cmd;
        stream >> task_cmd;

        if (task_cmd.compare("index") == 0) {
            spdlog::info("Handling parition commnad...");

            std::string job_id = gen_random(16);
            std::shared_ptr<PartitionJob> prt_job = std::make_shared<PartitionJob>(msg, job_id, client_id,
                                                                                   driver);
            spdlog::info("Created parition job object");
            driver->GetExecutor()->AddJob(prt_job);
        } else if (task_cmd.compare("dsp") == 0) {
            spdlog::info("Handling dispatch commnad...");
            std::string job_id = gen_random(16);
            std::shared_ptr<DispatchJob> dsp_job = std::make_shared<DispatchJob>(msg, job_id, client_id,
                                                                                 driver);
            spdlog::info("Created dispatch job object");
            driver->GetExecutor()->AddJob(dsp_job);
        } else if (task_cmd.compare("ls") == 0) {
            spdlog::info("Handling list commnad...");
            indices_lock.lock();

            std::stringstream ls_response;
            ls_response << "ID\t\tSOURCE\t\tPARTITIONS\n";
            for (auto &idx : indices) {
                ls_response << idx->Print() << '\n';
            }
            ls_response << "\n("
                        << indices.size() << ") indices";
            indices_lock.unlock();
            driver->SendToClient(client_id, "MSG", ls_response.str());
        } else {
            spdlog::info("Unknown Command : [{}]", task_cmd);
            driver->SendToClient(client_id, "MSG Unknown command");
        }
    });

    driver->Start();
    return 0;
}