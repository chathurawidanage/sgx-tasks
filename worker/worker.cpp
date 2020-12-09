#include "include/worker.hpp"

tasker::Worker::Worker(const std::string &id) : id(id) {
}

void tasker::Worker::OnMessage(const std::function<void(std::string)> &on_message) {
    this->on_message = on_message;
}

const std::string &tasker::Worker::GetId() {
    return this->id;
}

void tasker::Worker::Send(const std::string &cmd, const std::string &msg) const {
    std::string final_msg;
    final_msg.reserve(cmd.size() + 1 + id.size() + 1 + msg.size());
    final_msg.append(cmd);
    final_msg.append(SPACE);
    final_msg.append(id);

    if (msg.size() > 0) {
        final_msg.append(SPACE);
        final_msg.append(msg);
    }

    zmq::message_t message(final_msg.size());
    std::memcpy(message.data(), final_msg.data(), final_msg.size());

    spdlog::info("Sending message : {}", message.to_string());
    socket->send(message, zmq::send_flags::none);
}

int64_t tasker::Worker::GetPingInterval() {
    return this->ping_interval;
}

int tasker::Worker::Start(std::string &driver_address) {
    spdlog::info("Starting worker {}", this->id);

    //connect to the driver
    zmq::context_t ctx{1};  // 1 IO thread

    this->socket = new zmq::socket_t{ctx, zmq::socket_type::dealer};
    this->socket->setsockopt(ZMQ_IDENTITY, this->id.c_str(), this->id.size());

    spdlog::info("Connecting to the driver at {}", driver_address);
    this->socket->connect(driver_address);

    if (this->socket->connected()) {
        spdlog::info("Connected to the server...");
    } else {
        spdlog::info("Not connected to the server...");
        return 500;
    }

    // sending join message
    Send(tasker::GetCommand(tasker::JOIN));

    // wait for acknowldgement
    zmq::message_t request;
    socket->recv(request, zmq::recv_flags::none);
    if (tasker::GetCommand(tasker::Commands::ACK).compare(request.to_string()) == 0) {
        spdlog::info("Registration acknowledgement recieved from the driver");
    } else {
        spdlog::error("Invalid message received as the acknowledgement from the driver");
        return 500;
    }

    // start ping thread
    auto ping_thread = std::thread([&]() {
        std::string ping_cmd = tasker::GetCommand(tasker::Commands::PING);
        while (true) {
            this->Send(ping_cmd);
            std::this_thread::sleep_for(std::chrono::milliseconds(this->GetPingInterval()));
        }
    });

    // now start continuous listening
    while (true) {
        // receive a request from client
        spdlog::debug("Waiting for command..");
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
        } else {
            spdlog::info("Unknown message : {}", msg);
        }
    }
    ping_thread.join();
    return 0;
}