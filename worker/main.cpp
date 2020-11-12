#include <iostream>

#include "zmq.hpp"

class Worker {
   private:
    std::string COMMAND_EXE = "EXE";
    zmq::socket_t *socket;
    const std::string &id;

   public:
    Worker(const std::string &id) : id(id) {
    }

    int execute(std::string &command) {
        std::cout << "Executing command " << command << std::endl;
        return 0;
    }

    int start() {
        //connect to the driver
        zmq::context_t ctx{1};  // 1 IO thread

        this->socket = new zmq::socket_t{ctx, zmq::socket_type::dealer};
        this->socket->setsockopt(ZMQ_IDENTITY, this->id.c_str(), 7);

        std::cout << "Connecting to the driver..." << std::endl;
        this->socket->connect("tcp://localhost:5555");

        if (socket->connected()) {
            std::cout << "Connected to the server..." << std::endl;
        } else {
            std::cout << "Not connected to the server..." << std::endl;
            return 500;
        }

        socket->send(zmq::buffer("JOIN A"), zmq::send_flags::none);

        while (true) {
            zmq::message_t request;

            // receive a request from client
            std::cout << "Waiting for command.." << std::endl;
            socket->recv(request, zmq::recv_flags::none);

            std::string msg = request.to_string();
            std::string cmd = msg.substr(0, 3);
            std::string params = msg.substr(3, msg.length());

            int status = -1;
            // blocking execution
            if (COMMAND_EXE.compare(cmd) == 0) {
                status = execute(params);
            } else {
                std::cout << "Unknown message : " << msg << std::endl;
            }
            std::string response = "RSP " + std::to_string(status);
            zmq::message_t message(response.size());
            std::memcpy(message.data(), response.data(), response.size());
            socket->send(message, zmq::send_flags::none);
        }
        return 0;
    }
};

int main(int argc, char *argv[]) {
    Worker worker("WorkerA");
    worker.start();
    return 0;
}