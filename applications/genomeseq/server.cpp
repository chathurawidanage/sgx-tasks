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
#include "index.hpp"
#include "job.hpp"
#include "jobs/job_handlers.hpp"
#include "messages.hpp"
#include "spdlog/spdlog.h"
#include "uuid.hpp"
#include "worker_handler.hpp"
#include "worker_types.hpp"

std::string root_dir = get_root();

std::mutex indices_lock;
std::vector<std::shared_ptr<Index>> indices{};

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
            HandleIndex(msg, client_id, driver, std::make_shared<std::function<void(int32_t, std::string, std::string)>>([&](int32_t partitions, std::string index_id, std::string src_file) {
                            indices_lock.lock();
                            indices.push_back(std::make_shared<Index>(partitions, index_id, src_file));
                            indices_lock.unlock();
                        }));
        } else if (task_cmd.compare("dsp") == 0) {
            HandleDispatch(msg, client_id, driver);
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