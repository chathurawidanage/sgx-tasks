FROM tozd/sgx:ubuntu-xenial

RUN apt-get update
RUN apt-get -y install libzmq3-dev libspdlog-dev cmake make git build-essential nano iputils-ping net-tools
# spdlog
RUN git clone https://github.com/gabime/spdlog.git
RUN cd spdlog && mkdir build && cd build
RUN cd spdlog && cmake .. && make -j

RUN git clone --recurse-submodules https://github.com/chathurawidanage/sgx-tasks.git
RUN cd sgx-tasks
RUN /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc -H/sgx-tasks -B/sgx-tasks/build -G "Unix Makefiles"
RUN /usr/bin/cmake --build /sgx-tasks/build --config Release --target all -- -j 10
# indexing
RUN apt-get -y install bwa
# parition
RUN apt-get -y install python3
ADD python-util python-util
EXPOSE 5050 5000


