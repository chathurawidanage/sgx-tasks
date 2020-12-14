#include <cxxopts.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "commands.hpp"
#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker.hpp"

std::string prt_command = "prt";
std::string idx_command = "idx";

std::string msg_cmd = tasker::GetCommand(tasker::Commands::MESSAGE);

int main(int argc, char *argv[]) {
    std::string root_dir = get_root();

    spdlog::info("Using {} as the root directory", root_dir);

    tasker::Worker worker(gen_random(16));
    worker.OnMessage([&worker, &root_dir](std::string msg) {
        spdlog::info("Message received from server : {}", msg);

        // validation variables
        int32_t validation_code;
        std::string validation_msg;

        std::string cmd = msg.substr(0, 3);
        if (cmd.compare(prt_command) == 0) {
            try {
                spdlog::info("Handling partition command...");

                auto partition_command = PartitionCommand(msg);
                partition_command.Parse(&validation_code, &validation_msg);

                if (validation_code != 0) {
                    worker.Send(msg_cmd, validation_msg);
                    return;
                }

                std::string sys_command = "python3 /bio-sgx/split.py " + partition_command.GetSrcFile() + " " + std::to_string(partition_command.GetPartitions()) + " " + partition_command.GetDstFolder();
                spdlog::info("Executing command {}", sys_command);
                int status = system(sys_command.c_str());

                std::string resp = create_response(status);
                worker.Send(msg_cmd, resp);
                spdlog::info("Sent response to driver {}", resp);
            } catch (cxxopts::option_has_no_value_exception &err) {
                std::string error_msg = "Invalid command for partitioning : ";
                error_msg.append(err.what());
                spdlog::error(error_msg);

                std::string resp = create_response(500, error_msg);
                worker.Send(msg_cmd, resp);
            }
        } else if (cmd.compare(idx_command) == 0) {
            spdlog::info("Handling index command...");
            auto index_command = IndexCommand(msg);
            index_command.Parse(&validation_code, &validation_msg);

            if (validation_code != 0) {
                worker.Send(msg_cmd, validation_msg);
                return;
            }

            std::string sys_command = "/bwa/bwa index " + index_command.GetSrcFile();
            spdlog::info("Executing command {}", sys_command);
            int status = system(sys_command.c_str());

            std::string resp = create_response(status);
            worker.Send(msg_cmd, resp);
            spdlog::info("Sent response to driver {}", resp);
        } else {
            std::string resp = create_response(404, "Unknown command " + msg_cmd);
            worker.Send(msg_cmd, resp);
        }
    });
    std::string server_url = "tcp://localhost:5050";
    if (argc == 2) {
        server_url = argv[1];
    }
    worker.Start(server_url);
    return 0;
}