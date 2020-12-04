#ifndef F794764F_49EB_4263_8044_FD6BE9B4B3DD
#define F794764F_49EB_4263_8044_FD6BE9B4B3DD
#include <spdlog/spdlog.h>

#include <functional>
#include <set>
#include <string>
#include <zmq.hpp>

namespace tasker {

class JobExecutor;

class Driver {
   private:
    std::function<void(std::string &, std::string &)> on_worker_joined;
    std::function<void(std::string &, std::string &)> on_client_connected;

    std::function<void(std::string &, std::string &)> on_worker_msg;
    std::function<void(std::string &, std::string &)> on_client_msg;

    std::shared_ptr<zmq::socket_t> client_socket;
    std::shared_ptr<zmq::socket_t> worker_socket;

    std::shared_ptr<tasker::JobExecutor> executor;

    void StartHandler(int32_t port,
                      std::shared_ptr<zmq::socket_t> &socket,
                      const std::function<void(std::string &, std::string &)> &on_connected,
                      const std::function<void(std::string &, std::string &)> &on_msg,
                      const std::function<void(std::string &)> &on_ping);

    void Send(std::shared_ptr<zmq::socket_t> socket, const std::string &to, const std::string &msg) const;

   public:
    std::shared_ptr<tasker::JobExecutor> GetExecutor() {
        return this->executor;
    }

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

    void Start();

    Driver() = default;
};
}  // namespace tasker
#endif /* F794764F_49EB_4263_8044_FD6BE9B4B3DD */
