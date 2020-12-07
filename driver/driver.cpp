#include "driver.hpp"

#include "executor.hpp"
#include "messages.hpp"
#include "zmq.hpp"

void tasker::Driver::Start() {
    this->executor = std::make_shared<tasker::JobExecutor>(*this);

    std::thread cl_trd = std::thread(&Driver::StartHandler, this, 5000, std::ref(this->client_socket),
                                     this->on_client_connected, this->on_client_msg, nullptr, false);
    std::thread wk_trd = std::thread(
        &Driver::StartHandler, this, 5050, std::ref(this->worker_socket),
        this->on_worker_joined, this->on_worker_msg, [this](std::string &worker_id) {
            this->GetExecutor()->OnPing(worker_id);
        },
        true);
    this->executor->Start();
    cl_trd.join();
    wk_trd.join();
}

void tasker::Driver::Send(std::shared_ptr<zmq::socket_t> socket, const std::string &to, const std::string &msg) const {
    try {
        spdlog::info("Sending message : {} to {}", msg, to);
        zmq::message_t header_msg(to.size());
        std::memcpy(header_msg.data(), to.data(), to.size());
        socket->send(header_msg, zmq::send_flags::sndmore);

        zmq::message_t message(msg.size());
        std::memcpy(message.data(), msg.data(), msg.size());
        socket->send(message, zmq::send_flags::none);
    } catch (zmq::error_t &ex) {
        spdlog::error("Error in sending the message to the destination {}. Cause : {} {}", to, ex.what(), ex.num());
        if (ex.num() == 113) {  // host unreachable
            
        }
    }
}

void tasker::Driver::StartHandler(int32_t port,
                                  std::shared_ptr<zmq::socket_t> &socket,
                                  const std::function<void(std::string &, std::string &)> &on_connected,
                                  const std::function<void(std::string &, std::string &)> &on_msg,
                                  const std::function<void(std::string &)> &on_ping,
                                  bool worker_handler) {
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
            spdlog::debug("Registering remote process : [{}]", params);

            std::string m = tasker::GetCommand(tasker::Commands::ACK);
            this->Send(socket, params, m);

            std::string worker_type = "generic";
            if (worker_handler) {
                this->executor->AddWorker(params, worker_type);
            }

            if (on_connected != NULL) {
                on_connected(params, worker_type);
            }
        } else if (tasker::GetCommand(tasker::Commands::MESSAGE).compare(cmd) == 0) {
            spdlog::debug("Message received :  {}", params);
            std::string id = params.substr(0, params.find(' '));
            std::string rcvd_msg = params.substr(params.find(' ') + 1, params.size());
            // calling on message of user
            on_msg(id, rcvd_msg);
            if (worker_handler) {
                // calling on message of job
                this->executor->ForwardMsgToJob(id, rcvd_msg);
            }
        } else if (tasker::GetCommand(tasker::Commands::PING).compare(cmd) == 0) {
            spdlog::info("Ping received from {}", params);
            if (on_ping != nullptr) {
                on_ping(params);
            }
        } else {
            // this could be a pending join
            // spdlog::info("Unknown message : {}", request.to_string());
        }
    }
}