#include <functional>
#include <iostream>
#include <set>
#include <thread>
#include <unordered_map>

#include "messages.hpp"
#include "zmq.hpp"

namespace tasker {

class Worker {
   private:
    const std::string id;
    zmq::socket_t *socket;

    void send_str(const std::string &msg, zmq::send_flags flags) {
        zmq::message_t message(msg.size());
        std::memcpy(message.data(), msg.data(), msg.size());
        socket->send(message, flags);
    }

   public:
    Worker(zmq::socket_t *socket, std::string worker_id) : socket(socket), id(worker_id) {
        std::cout << "Created worker " << this->id << std::endl;
    }

    void Send(std::string &msg) {
        std::cout << "Send called... " << std::endl;
        std::cout << "Sending message to worker " << this->id << std::endl;
        this->send_str(this->id, zmq::send_flags::sndmore);
        this->send_str(msg, zmq::send_flags::none);
    }

    ~Worker() {
        std::cout << "Delete called..." << std::endl;
    }

    std::string Id() {
        return this->id;
    }

    int No() {
        return 101;
    }
};

class Driver {
   private:
    std::function<void(std::string, std::string)> on_worker_joined;
    std::function<void(std::string, std::string)> on_client_connected;

    void StartHandler(int32_t port, std::function<void(std::string, std::string)> on_connected) {
        std::unordered_map<std::string, Worker *> pending_joins{};

        zmq::context_t ctx{1};  // 1 IO thread

        // worker-driver communication
        zmq::socket_t socket{ctx, zmq::socket_type::router};
        socket.setsockopt(ZMQ_ROUTER_MANDATORY, 1);
        std::string address = "tcp://*:" + std::to_string(port);
        socket.bind(address);

        std::cout << "Binding to the socket " << address << std::endl;

        while (true) {
            // send hello
            zmq::message_t request;

            // receive a request from client
            socket.recv(request, zmq::recv_flags::none);

            std::string msg = request.to_string();

            std::string cmd = msg.substr(0, 3);
            std::string params = msg.substr(4, msg.length());

            std::cout << "Command : " << cmd << ", Params : " << params << std::endl;

            if (tasker::GetCommand(tasker::Commands::JOIN).compare(cmd) == 0) {
                // check whether a pending join exists
                // Worker worker(&socket, pending_joins[params]);
                std::cout << "No of registered workers :  " << pending_joins.size() << std::endl;
                std::cout << "Looking or target Id : [" << params << "]" << std::endl;

                std::unordered_map<std::string, Worker *>::iterator it = pending_joins.find(params);

                if (it == pending_joins.end()) {
                    std::cout << "Couldn't find worker " << params << std::endl;
                } else {
                    std::string m = tasker::GetCommand(tasker::Commands::ACK);
                    pending_joins[params]->Send(m);
                }

                if (on_connected != NULL) {
                    on_connected(params, "");
                }
            } else if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
                std::cout << "Message received from worker :  " << params << std::endl;
            } else {
                std::string worker_id = request.to_string();
                // this could be a pending join
                std::cout << "Registering the first message from [" << worker_id << "]" << std::endl;

                Worker *worker = new Worker(&socket, worker_id);
                pending_joins.insert(std::make_pair(worker_id, worker));
                std::cout << "Added to pending joins" << std::endl;
            }
        }
    }

   public:
    void SetOnWorkerJoined(const std::function<void(std::string, std::string)> &on_worker_joined) {
        this->on_worker_joined = on_worker_joined;
    }

    void SetOnClientConnected(const std::function<void(std::string, std::string)> &on_client_connected) {
        this->on_client_connected = on_client_connected;
    }

    void Start() {
        std::thread clients(&Driver::StartHandler, this, 5000, this->on_client_connected);
        std::thread workers(&Driver::StartHandler, this, 5050, this->on_worker_joined);

        clients.join();
        workers.join();
    }
};
}  // namespace tasker

int main(int argc, char *argv[]) {
    tasker::Driver driver;
    driver.SetOnWorkerJoined([](std::string worker_id, std::string worker_type) {
        std::cout << "Worker joined : " << worker_id << std::endl;
    });

    driver.SetOnClientConnected([](std::string client_id, std::string client_meta) {
        std::cout << "Client connected : " << client_id << std::endl;
    });
    driver.Start();
    return 0;
}