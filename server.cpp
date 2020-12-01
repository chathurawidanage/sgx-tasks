#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include "driver.hpp"
#include "executor.hpp"
#include "job.hpp"
#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker_handler.hpp"

class IndexJob : public tasker::Job {
    std::string commnd;

   public:
    IndexJob(std::string command, std::string job_id, std::string client_id,
             tasker::Driver &driver) : Job(job_id, client_id, driver) {
        this->commnd = command;
    }

    
};

/**
 * This job will require only one worker
 * */
class PartitionJob : public tasker::Job {
    std::shared_ptr<tasker::WorkerHandler> worker = nullptr;
    std::string commnd;

    bool job_done = false;

   public:
    PartitionJob(std::string command, std::string job_id, std::string client_id,
                 tasker::Driver &driver) : Job(job_id, client_id, driver) {
        this->commnd = command;
    }

    void OnWorkerMessage(std::string &worker_id, std::string &msg) {
        spdlog::info("Message from worker {}, {}", worker_id, msg);
        this->job_done = true;

        // sending the message back to the client
        std::string rsp_msg;
        rsp_msg.append("MSG ").append(msg);
        driver.SendToClient(this->client_id, rsp_msg);
    }

    bool Progress() {
        if (worker == nullptr) {
            this->worker = driver.GetExecutor()->AllocateWorker(*this, "");
            if (this->worker != nullptr) {
                spdlog::info("Allocated worker {} to job {}", this->worker->GetId(), this->job_id);

                spdlog::info("Sending command to worker {}", this->commnd);
                this->worker->Send(this->commnd);
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

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::debug);
    // spdlog::set_pattern("[%H:%M:%S %z] [%l] [trd %t] %v");

    tasker::Driver driver;

    driver.SetOnWorkerJoined([&driver](std::string &worker_id, std::string &worker_type) {
        spdlog::info("Worker joined : {}", worker_id);
        driver.GetExecutor()->AddWorker(worker_id, worker_type);
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
        }
    });

    driver.Start();
    return 0;
}