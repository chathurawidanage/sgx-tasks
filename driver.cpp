#include <functional>
#include <iostream>
#include <set>
#include <thread>
#include <unordered_map>

#include "messages.hpp"
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
        std::unordered_map<std::string, Worker *> pending_joins{};

        zmq::context_t ctx{1};  // 1 IO thread

        // worker-driver communication
        socket = std::make_shared<zmq::socket_t>(ctx, zmq::socket_type::router);
        socket->setsockopt(ZMQ_ROUTER_MANDATORY, 1);
        std::string address = "tcp://*:" + std::to_string(port);
        socket->bind(address);

        std::cout << "Binding to the socket " << socket.use_count() << address << std::endl;

        while (true) {
            // send hello
            zmq::message_t request;

            // receive a request from client
            std::cout << "Blocking for a message..." << std::endl;
            socket->recv(request, zmq::recv_flags::none);
            std::cout << "Recvd message : " << request.to_string() << std::endl;

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
                    this->Send(socket, params, m);
                }

                if (on_connected != NULL) {
                    std::string w_type = "generic";
                    on_connected(params, w_type);
                }
            } else if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
                std::cout << "Message received from worker :  " << params << std::endl;
                std::string id = params.substr(0, params.find(' '));
                std::string rcvd_msg = params.substr(params.find(' '), params.size());

                on_msg(id, rcvd_msg);
            } else {
                std::string worker_id = request.to_string();
                // this could be a pending join
                std::cout << "Registering the first message from [" << worker_id << "]" << std::endl;

                Worker *worker = new Worker(nullptr, worker_id);
                pending_joins.insert(std::make_pair(worker_id, worker));
                std::cout << "Added to pending joins" << std::endl;
            }
        }
    }

    void Send(std::shared_ptr<zmq::socket_t> socket, const std::string &to, const std::string &msg) const {
        std::cout << "Sending message : " << msg << " to : " << to << std::endl;
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

int main(int argc, char *argv[]) {
    tasker::Driver driver;

    driver.SetOnWorkerJoined([&driver](std::string &worker_id, std::string &worker_type) {
        std::cout << "Worker joined : " << worker_id << std::endl;
        driver.SendToWorker(worker_id, "MSG DSP");
    });

    driver.SetOnClientConnected([](std::string &client_id, std::string &client_meta) {
        std::cout << "Client connected : " << client_id << std::endl;
    });

    driver.SetOnWorkerMsg([&driver](std::string &worker_id, std::string &msg) {
        std::cout << "Worker message : " << worker_id << " : " << msg << std::endl;
    });

    driver.SetOnClientMsg([&driver](std::string &client_id, std::string &msg) {
        std::cout << "Clinet message : " << client_id << " : " << msg << std::endl;
        driver.SendToClient(client_id, "Gotcha!");
    });

    driver.Start();
    return 0;
}