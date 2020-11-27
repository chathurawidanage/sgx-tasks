#include "driver/include/driver.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include "driver/include/job.hpp"
#include "driver/include/worker.hpp"
#include "spdlog/spdlog.h"

/**
 * This job will require only one worker
 * */
class PartitionJob : public tasker::Job {
    std::shared_ptr<tasker::Worker> worker = NULL;

    void OnWorkerMessage(std::string &worker_id, std::string &msg) {
    }

    bool Progress() {
        if (worker == NULL) {
        }
        return false;
    }

    void Finalize() {
    }
};

int main(int argc, char *argv[]) {
    //spdlog::set_level(spdlog::level::debug);

    std::queue<std::string> available_workers{};

    tasker::Driver driver;

    driver.SetOnWorkerJoined([&driver, &available_workers](std::string &worker_id, std::string &worker_type) {
        spdlog::info("Worker joined : {}", worker_id);
        //driver.SendToWorker(worker_id, "MSG DSP");
        available_workers.push(worker_id);
    });

    driver.SetOnClientConnected([](std::string &client_id, std::string &client_meta) {
        spdlog::info("Client connected : {}", client_id);
    });

    driver.SetOnWorkerMsg([&driver](std::string &worker_id, std::string &msg) {
        spdlog::info("Worker message from {} : {}", worker_id, msg);
    });

    driver.SetOnClientMsg([&driver, &available_workers](std::string &client_id, std::string &msg) {
        spdlog::info("Clinet message from {} : {}", client_id, msg);
        driver.SendToClient(client_id, "Gotcha!");
        std::string worker_id = available_workers.front();
        driver.SendToWorker(worker_id, msg);
    });

    driver.Start();
    return 0;
}