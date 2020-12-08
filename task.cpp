#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker.hpp"

int main(int argc, char *argv[]) {
    tasker::Worker worker(gen_random(16));
    worker.OnMessage([&worker](std::string msg) {
        spdlog::info("Message received from server : {}", msg);
        std::string cmd = tasker::GetCommand(tasker::Commands::MESSAGE);
        std::string resp = "";

        std::istringstream commnad(msg);
        vector<string> tokens{istream_iterator<string>{commnad},
                              istream_iterator<string>{}};
        if (tokens[0].compare("prt") == 0) {
            std::string sys_command = "python3 /bio-sgx/split.py " + tokens[3] + " " + tokens[4] + " " + tokens[5];
            spdlog::info("Executing command {}", sys_command);
            int status = system(sys_command.c_str());
            resp.append(std::to_string(status));
            worker.Send(cmd, resp);
        } else {
            resp.append(" Unknown command ");
            resp.append(tokens[0]);
            worker.Send(cmd, resp);
        }
    });
    std::string server_url = "tcp://localhost:5050";
    if (argc == 2) {
        server_url = argv[1];
    }
    worker.Start(server_url);
    return 0;
}