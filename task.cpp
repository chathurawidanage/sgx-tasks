#include <cxxopts.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker.hpp"

std::string create_response(int32_t error_code, std::string msg = "") {
    std::string rsp;
    rsp.append("MSG ");
    rsp.append(std::to_string(error_code));
    if (msg.size() > 0) {
        rsp.append(" ");
        rsp.append(msg);
    }
    return rsp;
}

int main(int argc, char *argv[]) {
    std::string root_dir = "";
    if (std::getenv("ENV_ROOT_PATH") != nullptr) {
        root_dir = std::getenv("ENV_ROOT_PATH");
    }

    spdlog::info("Using {} as the root directory", root_dir);

    tasker::Worker worker(gen_random(16));
    worker.OnMessage([&worker, &root_dir](std::string msg) {
        spdlog::info("Message received from server : {}", msg);
        std::string cmd = tasker::GetCommand(tasker::Commands::MESSAGE);

        std::istringstream commnad(msg);
        vector<std::string> tokens{istream_iterator<string>{commnad},
                                   istream_iterator<string>{}};
        const char *prt_command = "prt";
        if (std::strcmp(tokens[0].c_str(), prt_command) == 0) {
            try {
                spdlog::info("Handling partition command...");
                vector<const char *> args(tokens.size());
                std::transform(tokens.begin(), tokens.end(), args.begin(), [](std::string &tkn) {
                    return tkn.c_str();
                });

                cxxopts::Options options("Parition", "Parition Command Handler");
                options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>())("d,destination", "Destination folder", cxxopts::value<std::string>());

                auto results = options.parse(args.size(), args.data());

                std::string src_file = root_dir + results["s"].as<std::string>();
                std::string dst_folder = root_dir + results["d"].as<std::string>();
                int32_t partitions = results["p"].as<std::int32_t>();

                // check source file exists
                if (!std::filesystem::exists(src_file)) {
                    std::string resp = create_response(404, "File " + src_file + " doesn't exists");
                    worker.Send(cmd, resp);
                    return;
                }

                spdlog::info("Creating output directories {}", dst_folder);

                // check output directory exists
                std::filesystem::create_directories(dst_folder);

                spdlog::info("Command : p : {}, s: {}, d: {}", results["p"].as<int32_t>(), results["s"].as<std::string>(), results["d"].as<std::string>());

                std::string sys_command = "python3 /bio-sgx/split.py " + src_file + " " + std::to_string(partitions) + " " + dst_folder;
                spdlog::info("Executing command {}", sys_command);
                int status = system(sys_command.c_str());

                std::string resp = create_response(status);
                worker.Send(cmd, resp);
                spdlog::info("Sent response to driver {}", resp);
            } catch (cxxopts::option_has_no_value_exception &err) {
                std::string error_msg = "Invalid command for partitioning : ";
                error_msg.append(err.what());
                spdlog::error(error_msg);

                std::string resp = create_response(500, error_msg);
                worker.Send(cmd, resp);
            }
        } else {
            std::string resp = create_response(404, "Unknown command " + tokens[0]);
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