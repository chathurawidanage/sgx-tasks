#include <chrono>
#include <iostream>
#include <string>

#include "messages.hpp"
#include "zmq.hpp"

namespace tasker {
class Client {
  private:
    std::string SPACE = " ";
    zmq::socket_t *socket;
    const std::string id;
    const std::string server_url;

    void Send(const std::string &cmd, const std::string &msg = "") {
        std::string final_message;
        int32_t msg_length = cmd.size() + 1 + this->id.size();
        if (msg.size() > 0) {
            msg_length += (1 + msg.size());
        }
        final_message.reserve(msg_length);

        final_message.append(cmd);
        final_message.append(" ");
        final_message.append(this->id);
        if (msg.size() > 0) {
            final_message.append(" ");
            final_message.append(msg);
        }

        zmq::message_t message(final_message.size());
        std::memcpy(message.data(), final_message.data(), final_message.size());

        // std::cout << "Sending message : " << message.to_string() << std::endl;
        socket->send(message, zmq::send_flags::none);
    }

  public:
    Client(const std::string &id, const std::string &server_url) : id(id), server_url(server_url) {}

    int ExecAndDie(std::string command) { Start(true, command); }

    int Start() { return Start(false, ""); }

    int Start(bool execOnce, std::string command) {
        // connect to the driver
        zmq::context_t ctx{1}; // 1 IO thread

        this->socket = new zmq::socket_t{ctx, zmq::socket_type::dealer};
        this->socket->setsockopt(ZMQ_IDENTITY, this->id.c_str(), this->id.size());

        std::cout << "Connecting to the driver..." << std::endl;
        this->socket->connect(this->server_url);

        if (socket->connected()) {
            std::cout << "Connected to the server..." << std::endl;
        } else {
            std::cout << "Not connected to the server..." << std::endl;
            return 500;
        }

        // sending join message
        std::string cmd = tasker::GetCommand(tasker::JOIN);
        Send(cmd);

        // wait for ack
        zmq::message_t ack_req;
        socket->recv(ack_req, zmq::recv_flags::none);
        if (tasker::GetCommand(tasker::Commands::ACK).compare(ack_req.to_string()) == 0) {
            std::cout << "Ack recevied for join" << std::endl;
        } else {
            std::cout << "Ack not recevied for join" << std::endl;
            return 500;
        }

        cmd = tasker::GetCommand(tasker::MESSAGE);

        bool update_msg = false;

        auto start = std::chrono::high_resolution_clock::now();

        // now start continuous listening
        do {
            std::string line = command;
            if (!update_msg) {
                if (!execOnce) {
                    std::cout << "client$ ";
                    std::getline(std::cin, line);
                } else {
                    std::cout << "Executing command : " << line << std::endl;
                }
                start = std::chrono::high_resolution_clock::now();
                Send(cmd, line);
            } else {
                update_msg = false;
            }

            zmq::message_t request;

            // receive a request from client
            //std::cout << "Waiting for response.." << std::endl;
            socket->recv(request, zmq::recv_flags::none);
            auto stop = std::chrono::high_resolution_clock::now();

            std::string msg = request.to_string();
            std::string cmd = msg.substr(0, 3);
            std::string params = "";

            if (msg.size() > 3) {
                params = msg.substr(4, msg.length());
            }

            int status = -1;
            if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
                auto end = std::chrono::high_resolution_clock::now();
                std::cout << "Command Execution Time : " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "sec."<<std::endl;
                std::cout << params << std::endl;
            } else if (tasker::GetCommand(tasker::Commands::UPDATE).compare(cmd) == 0) {
                std::cout << params << std::endl;
                update_msg = true;
            } else {
                std::cout << "Unknown message : " << msg << std::endl;
            }
            //std::cout << "Command completed in " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << "s" << std::endl;
        } while (!execOnce || update_msg);
        return 0;
    }
};
} // namespace tasker

int main(int argc, char *argv[]) {
    std::string server_url = "tcp://localhost:5000";
    if (argc >= 3) {
        server_url = argv[2];
    }
    tasker::Client client(argv[1], server_url);

    if (argc > 3) {
        client.ExecAndDie(argv[3]);
    } else {
        client.Start();
    }

    return 0;
}