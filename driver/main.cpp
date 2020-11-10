#include <iostream>
#include "zmq.hpp"
#include <unordered_map>

enum WorkerType
{
    secure,
    unsecure
};

class Worker
{
private:
    WorkerType type;
};

class Server
{
private:
    std::unordered_map<WorkerType, std::unordered_map<int32_t, Worker *>> all_workers{};
    std::unordered_map<WorkerType, std::unordered_map<int32_t, Worker *>> busy_workers{};
    std::unordered_map<WorkerType, std::unordered_map<int32_t, Worker *>> available_workers{};
};

int main(int argc, char *argv[])
{

    zmq::context_t ctx{1}; // 1 IO thread

    zmq::socket_t socket{ctx, zmq::socket_type::router};
    socket.setsockopt(ZMQ_ROUTER_MANDATORY, 1);
    socket.bind("tcp://*:5555");

    getchar();

    int count = 0;
    while (true)
    {
        // send hello
        zmq::message_t request;

        // receive a request from client
        std::cout << "Waiting for dealer.." << std::endl;
        socket.recv(request, zmq::recv_flags::none);
        std::cout << "Msg from dealer" << request.to_string() << ", " << request.str() << std::endl;

        std::cout << "Sending message..." << std::endl;
        if (count == 0)
        {
            zmq::message_t response;
            response.copy(&request);
            socket.send(response, ZMQ_SNDMORE);
        }
        else
        {
            socket.send(zmq::str_buffer("Hello from server"), zmq::send_flags::none);
        }
        count++;
    }

    return 0;
}