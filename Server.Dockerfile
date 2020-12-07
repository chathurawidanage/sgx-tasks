FROM ubuntu:20.10

RUN apt-get update
RUN apt-get -y install libzmq3-dev libspdlog-dev cmake make git build-essential nano iputils-ping net-tools
RUN ls /etc
RUN git clone https://github.com/chathurawidanage/sgx-tasks.git
RUN cd sgx-tasks
RUN /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/bin/gcc -H/sgx-tasks -B/sgx-tasks/build -G "Unix Makefiles"
RUN /usr/bin/cmake --build /sgx-tasks/build --config Release --target all -- -j 10
EXPOSE 5050 5000


