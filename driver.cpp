#include <iostream>
#include <set>
#include <unordered_map>

#include "messages.hpp"
#include "zmq.hpp"

namespace tasker {

class Worker {
   private:
    zmq::messagef_t route_message;
    zmq::socket_t *socket;

   public:
    Worker(zmq::socket_t *socket, zmq::message_t &base_message) : socket(socket) {
        route_message.move(base_message);
    }

    void send(std::string &msg) {
        std::cout << "Sending message " << msg << std::endl;
        std::cout << "Sent headers. Sending actual message..." << route_message.to_string() << std::endl;

        socket->send(route_message, zmq::send_flags::sndmore);

        zmq::message_t message(msg.size());
        std::memcpy(message.data(), msg.data(), msg.size());
        socket->send(message, zmq::send_flags::none);
    }
};

class Driver {
   private:
    std::set<std::string> worker_types{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> all_workers{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> busy_workers{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> available_workers{};

    std::unordered_map<std::string, Worker *> pending_joins{};

   public:
    void start() {
        zmq::context_t ctx{1};  // 1 IO thread

        zmq::socket_t socket{ctx, zmq::socket_type::router};
        socket.setsockopt(ZMQ_ROUTER_MANDATORY, 1);
        socket.bind("tcp://*:5555");

        while (true) {
            // send hello
            zmq::message_t request;

            // receive a request from client
            socket.recv(request, zmq::recv_flags::none);

            std::string msg = request.to_string();

            std::string cmd = msg.substr(0, 3);
            std::string params = msg.substr(3, msg.length());

            std::cout << "Command : " << cmd << ", Params : " << params << std::endl;

            if (tasker::GetCommand(tasker::Commands::JOIN).compare(cmd) == 0) {
                // check whether a pending join exists
                // Worker worker(&socket, pending_joins[params]);
                std::string m = "Hi from server";
                pending_joins[params]->send(m);
            } else {
                // this could be a pending join
                std::cout << "Registering the first message..." << std::endl;
                Worker *worker = new Worker(&socket, request);
                this->pending_joins.insert(std::make_pair(msg, worker));
                std::cout << "Added to pending joins" << std::endl;
            }
        }
    }
};
}  // namespace tasker

int main(int argc, char *argv[]) {
    tasker::Driver driver;
    driver.start();
    return 0;
}