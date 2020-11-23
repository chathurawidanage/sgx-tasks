#include <functional>
#include <iostream>
#include <string>

#include "messages.hpp"
#include "zmq.hpp"

namespace tasker {
class Worker {
   private:
    std::string SPACE = " ";
    zmq::socket_t *socket;
    const std::string id;
    std::function<void(std::string)> on_message;

   public:
    Worker(const std::string &id) : id(id) {
    }

    void SetOnMessage(const std::function<void(std::string)> &on_message) {
        this->on_message = on_message;
    }

    void Send(const std::string &cmd, const std::string &msg = "") const {
        std::string final_msg;
        final_msg.reserve(cmd.size() + 1 + id.size() + 1 + msg.size());
        final_msg.append(cmd);
        final_msg.append(" ");
        final_msg.append(id);

        if (msg.size() > 0) {
            final_msg.append(" ");
            final_msg.append(msg);
        }

        zmq::message_t message(final_msg.size());
        std::memcpy(message.data(), final_msg.data(), final_msg.size());

        std::cout << "Sending message : " << message.to_string() << std::endl;
        socket->send(message, zmq::send_flags::none);
    }

    int Start(std::string &driver_address) {
        std::cout << "Starting worker " << this->id << std::endl;

        //connect to the driver
        zmq::context_t ctx{1};  // 1 IO thread

        this->socket = new zmq::socket_t{ctx, zmq::socket_type::dealer};
        this->socket->setsockopt(ZMQ_IDENTITY, this->id.c_str(), this->id.size());

        std::cout << "Connecting to the driver..." << std::endl;
        this->socket->connect(driver_address);

        if (socket->connected()) {
            std::cout << "Connected to the server..." << std::endl;
        } else {
            std::cout << "Not connected to the server..." << std::endl;
            return 500;
        }

        // sending join message
        Send(tasker::GetCommand(tasker::JOIN));

        // now start continuous listening
        while (true) {
            zmq::message_t request;

            // receive a request from client
            std::cout << "Waiting for command.." << std::endl;
            socket->recv(request, zmq::recv_flags::none);

            std::string msg = request.to_string();
            std::string cmd = msg.substr(0, 3);
            std::string params = "";

            if (msg.size() > 3) {
                params = msg.substr(4, msg.length());
            }

            int status = -1;
            if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
                this->on_message(params);
            } else if (tasker::GetCommand(tasker::Commands::ACK).compare(cmd) == 0) {
                std::cout << "Ack received for connection..." << std::endl;
            } else {
                std::cout << "Unknown message : " << msg << std::endl;
            }
        }
        return 0;
    }
};
}  // namespace tasker

int main(int argc, char *argv[]) {
    tasker::Worker worker(argv[1]);
    worker.SetOnMessage([&worker](std::string msg) {
        std::cout << "Message received from server : " << msg << std::endl;
        std::string resp = "200";
        std::string cmd = tasker::GetCommand(tasker::Commands::MESSAGE);
        worker.Send(cmd, resp);
    });
    std::string driver_address = "tcp://localhost:5050";
    worker.Start(driver_address);
    return 0;
}