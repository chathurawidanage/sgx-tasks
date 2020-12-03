#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "spdlog/spdlog.h"
#include "worker.hpp"

int main(int argc, char *argv[]) {
    tasker::Worker worker(argv[1]);
    worker.OnMessage([&worker](std::string msg) {
        spdlog::info("Message received from server : {}", msg);
        std::string cmd = tasker::GetCommand(tasker::Commands::MESSAGE);

        spdlog::info("Waiting 10 seconds to simulate work...");

        std::string resp = "Partitioning done!";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        worker.Send(cmd, resp);
    });
    std::string server_url = "tcp://localhost:5050";
    if (argc == 3) {
        server_url = argv[2];
    }
    worker.Start(server_url);
    return 0;
}