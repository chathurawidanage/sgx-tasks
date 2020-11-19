#include <iostream>
#include <string>

#include "messages.hpp"
#include "zmq.hpp"

namespace tasker {
class Client {
   private:
    std::string SPACE = " ";
    zmq::socket_t *socket;
    const std::string &id;

    void Send(const std::string &msg) {
        std::cout << "Sending message : " << msg << std::endl;
        zmq::message_t message(msg.size());
        std::memcpy(message.data(), msg.data(), msg.size());
        socket->send(message, zmq::send_flags::none);
    }

   public:
    Client(const std::string &id) : id(id) {
    }

    int Start() {
        //connect to the driver
        zmq::context_t ctx{1};  // 1 IO thread

        this->socket = new zmq::socket_t{ctx, zmq::socket_type::dealer};
        this->socket->setsockopt(ZMQ_IDENTITY, this->id.c_str(), 7);

        std::cout << "Connecting to the driver..." << std::endl;
        this->socket->connect("tcp://localhost:5000");

        if (socket->connected()) {
            std::cout << "Connected to the server..." << std::endl;
        } else {
            std::cout << "Not connected to the server..." << std::endl;
            return 500;
        }

        // sending join message
        std::string cmd = tasker::GetCommand(tasker::JOIN);
        cmd.append(SPACE);
        cmd.append(this->id);
        Send(cmd);

        // now start continuous listening
        while (true) {
            std::string line;
            std::getline(std::cin, line);
            Send(line);

            zmq::message_t request;

            // receive a request from client
            std::cout << "Waiting for response.." << std::endl;
            socket->recv(request, zmq::recv_flags::none);

            std::string msg = request.to_string();
            std::string cmd = msg.substr(0, 3);
            std::string params = msg.substr(4, msg.length());

            int status = -1;
            if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
            } else {
                std::cout << "Unknown message : " << msg << std::endl;
            }
        }
        return 0;
    }
};
}  // namespace tasker

int main(int argc, char *argv[]) {
    tasker::Client client("random_client");
    client.Start();
    return 0;
}