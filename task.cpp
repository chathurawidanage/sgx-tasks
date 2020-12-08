#include <cxxopts.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker.hpp"

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
        std::string resp = "";

        std::istringstream commnad(msg);
        vector<std::string> tokens{istream_iterator<string>{commnad},
                                   istream_iterator<string>{}};
        const char *prt_command = "prt";
        if (std::strcmp(tokens[0].c_str(), prt_command) == 0) {
            vector<const char *> args(tokens.size());
            std::transform(tokens.begin(), tokens.end(), args.begin(), [](std::string &tkn) {
                return tkn.c_str();
            });

            cxxopts::Options options("Parition", "Parition Command Handler");
            options.allow_unrecognised_options();
            options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>())("d,destination", "Destination folder", cxxopts::value<std::string>());

            auto results = options.parse(args.size(), args.data());

            std::string src_file = results["s"].as<std::string>();
            std::string dst_folder = results["d"].as<std::string>();
            int32_t partitions = results["p"].as<std::int32_t>();

            // check source file exists
            if (!std::filesystem::exists(root_dir + src_file)) {
                resp.append("File " + src_file + " doesn't exists");
                worker.Send(cmd, resp);
                return;
            }

            // check output directory exists
            std::filesystem::create_directories(dst_folder);

            spdlog::info("Command : p : {}, s: {}, d: {}", results["p"].as<int32_t>(), results["s"].as<std::string>(), results["d"].as<std::string>());

            std::string sys_command = "python3 /bio-sgx/split.py " + src_file + " " + std::to_string(partitions) + " " + dst_folder;
            spdlog::info("Executing command {}", sys_command);
            int status = system(sys_command.c_str());
            resp.append(std::to_string(status));
            resp.append(" ");
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