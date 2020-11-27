#include <functional>
#include <iostream>
#include <string>

#include "spdlog/spdlog.h"
#include "worker.hpp"

int main(int argc, char *argv[]) {
    tasker::Worker worker(argv[1]);
    worker.OnMessage([&worker](std::string msg) {
        std::cout << "Message received from server : " << msg << std::endl;
        std::string resp = "200";
        std::string cmd = tasker::GetCommand(tasker::Commands::MESSAGE);
        worker.Send(cmd, resp);
    });
    std::string driver_address = "tcp://localhost:5050";
    worker.Start(driver_address);
    return 0;
}