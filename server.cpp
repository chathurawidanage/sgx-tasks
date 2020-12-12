#include <chrono>
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

std::string root_dir = get_root();

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

   public:
    IndexJob(std::string input_file,
             std::string job_id,
             std::string client_id,
             tasker::Driver &driver) : Job(job_id, client_id, driver) {
        this->input_file = input_file;
    }

    bool progress() {
        if (worker == nullptr) {
            this->worker = driver.GetExecutor()->AllocateWorker(*this, "");
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                std::string command = "bwa mem ";
                command.append(this->input_file);

                spdlog::info("Sending command to worker {}", command);
                this->worker->Send(command);
            } else {
                spdlog::info("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->job_done;
    }

    void Finalize() {
        spdlog::info("Finalizing job {}", this->job_id);
        if (this->worker != nullptr) {
            driver.GetExecutor()->ReleaseWorker(*this, this->worker);
        }
    }
};

/**
 * This job will require only one worker
 * */
class PartitionJob : public tasker::Job {
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;

    PartitionCommand *partition_command;  //todo delete

    bool job_done = false;

   public:
    PartitionJob(std::string command, std::string job_id, std::string client_id,
                 tasker::Driver &driver) : Job(job_id, client_id, driver) {
        std::string validation_msg;
        int32_t validation_code;
        partition_command = new PartitionCommand(command);
        partition_command->Parse(&validation_code, &validation_msg);

        if (validation_code != 0) {
            driver.SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), validation_msg);
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

        // sending the message back to the client
        driver.SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::MESSAGE), msg);
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
            this->worker = driver.GetExecutor()->AllocateWorker(*this, "");
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->partition_command->GetCommand());
                this->worker->Send(this->partition_command->GetCommand());
            } else {
                spdlog::info("Couldn't get a worker allocated for job {}", this->job_id);
            }
        }
        return this->job_done;
    }

    void Finalize() {
        spdlog::info("Finalizing job {}", this->job_id);
        if (this->worker != nullptr) {
            driver.GetExecutor()->ReleaseWorker(*this, this->worker);
        }
    }

    ~PartitionJob() {
        delete this->partition_command;
    }
};

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::debug);
    // spdlog::set_pattern("[%H:%M:%S %z] [%l] [trd %t] %v");

    tasker::Driver driver;

    driver.SetOnWorkerJoined([&driver](std::string &worker_id, std::string &worker_type) {
        spdlog::info("Worker joined : {}", worker_id);
    });

    driver.SetOnClientConnected([](std::string &client_id, std::string &client_meta) {
        spdlog::info("Client connected : {}", client_id);
    });

    driver.SetOnWorkerMsg([&driver](std::string &worker_id, std::string &msg) {
        spdlog::info("Worker message from {} : {}", worker_id, msg);
    });

    driver.SetOnClientMsg([&driver](std::string &client_id, std::string &msg) {
        spdlog::info("Clinet message from [{}] : {}", client_id, msg);
        // extracting out task
        std::istringstream stream(msg);
        std::string task_cmd;
        stream >> task_cmd;

        if (task_cmd.compare("prt") == 0) {
            spdlog::info("Handling parition commnad...");

            std::string job_id = gen_random(16);
            std::shared_ptr<PartitionJob> prt_job = std::make_shared<PartitionJob>(msg, job_id, client_id,
                                                                                   driver);
            spdlog::info("Created parition job object");
            driver.GetExecutor()->AddJob(prt_job);
        } else {
            spdlog::info("Unknown Command : {}", task_cmd);
            driver.SendToClient(client_id, "MSG Unknown command");
        }
    });

    driver.Start();
    return 0;
}