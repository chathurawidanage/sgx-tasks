FROM tozd/sgx:ubuntu-xenial

RUN apt-get update
RUN apt-get -y install libspdlog-dev make git build-essential nano iputils-ping net-tools

# zeromq
RUN git clone https://github.com/zeromq/libzmq.git
RUN cd libzmq && ./autogen.sh && ./configure && make && make install

# cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.15.2/cmake-3.15.2.tar.gz
RUN tar -zxvf cmake-3.15.2.tar.gz
RUN cd cmake-3.15.2 && ./bootstrap && make && make install

# tasker & sgx-dida
RUN git clone --recurse-submodules https://github.com/chathurawidanage/sgx-tasks.git
RUN cd sgx-tasks
RUN /cmake-3.15.2/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc -H/sgx-tasks -B/sgx-tasks/build -G "Unix Makefiles"
RUN /cmake-3.15.2/bin/cmake --build /sgx-tasks/build --config Release --target all -- -j 10

# indexing
RUN apt-get -y install bwa

# parition
RUN apt-get -y install python3
ADD python-util python-util


EXPOSE 5050 5000


