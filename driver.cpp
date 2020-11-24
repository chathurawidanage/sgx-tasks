#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include "messages.hpp"
#include "spdlog/spdlog.h"
#include "zmq.hpp"

namespace tasker {

class Driver {
   private:
    std::function<void(std::string &, std::string &)> on_worker_joined;
    std::function<void(std::string &, std::string &)> on_client_connected;

    std::function<void(std::string &, std::string &)> on_worker_msg;
    std::function<void(std::string &, std::string &)> on_client_msg;

    std::shared_ptr<zmq::socket_t> client_socket;
    std::shared_ptr<zmq::socket_t> worker_socket;

    void StartHandler(int32_t port,
                      std::shared_ptr<zmq::socket_t> &socket,
                      const std::function<void(std::string &, std::string &)> &on_connected,
                      const std::function<void(std::string &, std::string &)> &on_msg) {
        std::set<std::string> pending_joins{};
        std::set<std::string> joined{};

        zmq::context_t ctx{1};  // 1 IO thread

        // worker-driver communication
        socket = std::make_shared<zmq::socket_t>(ctx, zmq::socket_type::router);
        socket->setsockopt(ZMQ_ROUTER_MANDATORY, 1);
        std::string address = "tcp://*:" + std::to_string(port);
        socket->bind(address);

        spdlog::info("Binding to the socket {}", address);

        while (true) {
            // send hello
            zmq::message_t request;

            // receive a request from client
            spdlog::debug("Blocking for a message...");

            socket->recv(request, zmq::recv_flags::none);
            spdlog::debug("Recvd message : {}", request.to_string());

            std::string msg = request.to_string();

            std::string cmd = msg.substr(0, 3);
            std::string params = msg.substr(4, msg.length());

            spdlog::debug("Command : {}, Params : {}", cmd, params);

            if (tasker::GetCommand(tasker::Commands::JOIN).compare(cmd) == 0) {
                // check whether a pending join exists

                spdlog::debug("No of registered workers :  {}", pending_joins.size());
                spdlog::debug("Looking or target Id : [{}]", params);

                std::set<std::string>::iterator it = pending_joins.find(params);

                if (it == pending_joins.end()) {
                    spdlog::warn("Couldn't find worker {}", params);
                } else {
                    std::string m = tasker::GetCommand(tasker::Commands::ACK);
                    this->Send(socket, params, m);

                    // remove from pending and add to joined
                    pending_joins.erase(params);
                    joined.insert(params);
                }

                if (on_connected != NULL) {
                    std::string w_type = "generic";
                    on_connected(params, w_type);
                }
            } else if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
                spdlog::debug("Message received :  {}", params);
                std::string id = params.substr(0, params.find(' '));
                if (joined.find(id) != joined.end()) {
                    std::string rcvd_msg = params.substr(params.find(' '), params.size());
                    on_msg(id, rcvd_msg);
                } else {
                    spdlog::warn("Message received from an unknown worker {}", id);
                }
            } else {
                std::string worker_id = request.to_string();
                // this could be a pending join
                spdlog::info("Registering the first message from [{}]", worker_id);
                pending_joins.insert(worker_id);
            }
        }
    }

    void Send(std::shared_ptr<zmq::socket_t> socket, const std::string &to, const std::string &msg) const {
        spdlog::debug("Sending message : {} to {}", msg, to);
        zmq::message_t header_msg(to.size());
        std::memcpy(header_msg.data(), to.data(), to.size());
        socket->send(header_msg, zmq::send_flags::sndmore);

        zmq::message_t message(msg.size());
        std::memcpy(message.data(), msg.data(), msg.size());
        socket->send(message, zmq::send_flags::none);
    }

   public:
    void SendToClient(const std::string &to, const std::string &msg) const {
        this->Send(this->client_socket, to, msg);
    }

    void SendToWorker(const std::string &to, const std::string &msg) const {
        this->Send(this->worker_socket, to, msg);
    }

    void SetOnWorkerJoined(const std::function<void(std::string &, std::string &)> &on_worker_joined) {
        this->on_worker_joined = on_worker_joined;
    }

    void SetOnClientConnected(const std::function<void(std::string &, std::string &)> &on_client_connected) {
        this->on_client_connected = on_client_connected;
    }

    void SetOnWorkerMsg(const std::function<void(std::string &, std::string &)> &on_worker_msg) {
        this->on_worker_msg = on_worker_msg;
    }

    void SetOnClientMsg(const std::function<void(std::string &, std::string &)> &on_client_msg) {
        this->on_client_msg = on_client_msg;
    }

    void Start() {
        // sk1

        //sk2
        std::thread clients(&Driver::StartHandler, this, 5000, std::ref(this->client_socket),
                            this->on_client_connected, this->on_client_msg);
        std::thread workers(&Driver::StartHandler, this, 5050, std::ref(this->worker_socket),
                            this->on_worker_joined, this->on_worker_msg);

        clients.join();
        //workers.join();
    }

    Driver() = default;
};
}  // namespace tasker

class Job {
   private:
    std::string &client_id;

   public:
    Job(std::string &client_id) : client_id(client_id) {
    }

    /**
     * Returns true if job is completed
     * */
    bool Progress() {
        return false;
    }

    void Finalize() {
    }
};

class JobExecutor {
   private:
    std::list<Job> jobs{};
    std::mutex lock;

    void progress() {
        while (true) {
            if (jobs.empty()) {
                // todo replace with condition variables and locks
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            lock.lock();
            std::list<Job>::iterator i = jobs.begin();
            while (i != jobs.end()) {
                bool done = (*i).Progress();
                if (done) {
                    (*i).Finalize();
                    jobs.erase(i);
                }
                i++;
            }
            lock.unlock();
        }
    }

   public:
    void AddJob(Job &job) {
        lock.lock();
        this->jobs.push_back(job);
        lock.unlock();
    }

    void Start() {
        spdlog::info("Starting job executor...");
        std::thread(&JobExecutor::progress, this);
    }
};

int main(int argc, char *argv[]) {
    //spdlog::set_level(spdlog::level::debug);

    std::queue<std::string> available_workers{};
    std::queue<std::string> busy_workers{};

    tasker::Driver driver;

    JobExecutor executor;
    executor.Start();

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