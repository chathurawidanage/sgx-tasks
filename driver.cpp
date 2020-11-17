#include <iostream>
#include <set>
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
    std::set<std::string> worker_types{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> all_workers{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> busy_workers{};
    std::unordered_map<std::string, std::unordered_map<int32_t, Worker *>> available_workers{};

    std::unordered_map<std::string, Worker *> pending_joins{};

   public:
    void start() {
        zmq::context_t ctx{1};  // 1 IO thread

        // worker-driver communication
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
            std::string params = msg.substr(4, msg.length());

            std::cout << "Command : " << cmd << ", Params : " << params << std::endl;

            if (tasker::GetCommand(tasker::Commands::JOIN).compare(cmd) == 0) {
                // check whether a pending join exists
                // Worker worker(&socket, pending_joins[params]);
                std::cout << "No of registered workers :  " << pending_joins.size() << std::endl;
                std::cout << "Looking or target Id : [" << params << "]" << std::endl;

                std::unordered_map<std::string, Worker *>::iterator it = this->pending_joins.find(params);

                if (it == pending_joins.end()) {
                    std::cout << "Couldn't find worker " << params << std::endl;
                } else {
                    std::string m = "Hi from server";
                    pending_joins[params]->Send(m);
                }
            } else if (tasker::GetCommand(tasker::Commands::RESPONSE).compare(cmd) == 0) {
                std::cout << "Response received from worker :  " << params << std::endl;
            } else {
                std::string worker_id = request.to_string();
                // this could be a pending join
                std::cout << "Registering the first message from [" << worker_id << "]" << std::endl;

                Worker *worker = new Worker(&socket, worker_id);
                this->pending_joins.insert(std::make_pair(worker_id, worker));
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