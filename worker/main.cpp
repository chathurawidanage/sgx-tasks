#include <iostream>
#include "zmq.hpp"

int main(int argc, char *argv[])
{

    zmq::context_t ctx{1}; // 1 IO thread

    zmq::socket_t socket{ctx, zmq::socket_type::dealer};
    socket.setsockopt(ZMQ_IDENTITY, "A", 5);
    socket.connect("tcp://localhost:5555");

    if (socket.connected())
    {
        std::cout << "Connected to the server..." << std::endl;
    }
    else
    {
        std::cout << "Not connected to the server..." << std::endl;
    }

    while (true)
    {
        socket.send(zmq::buffer("Hello From Clinet"), zmq::send_flags::none);

        zmq::message_t request;

        // receive a request from client
        std::cout << "Waiting for command.." << std::endl;
        socket.recv(request, zmq::recv_flags::none);
        std::cout << "Received " << request.to_string() << std::endl;
    }

    return 0;
}