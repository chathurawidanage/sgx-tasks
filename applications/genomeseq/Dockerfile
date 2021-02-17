FROM ya0guang/sgx-u20:1.0

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install libfmt-dev libzmq3-dev libspdlog-dev cmake make git build-essential nano iputils-ping net-tools valgrind
RUN git clone --recurse-submodules https://github.com/chathurawidanage/sgx-tasks.git
RUN cd sgx-tasks git submodule update --init --recursive
RUN cd sgx-tasks/applications/genomeseq/dida-sgx/ && git checkout origin/tasker-integration
RUN /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/bin/gcc -H/sgx-tasks -B/sgx-tasks/build -G "Unix Makefiles"
RUN /usr/bin/cmake --build /sgx-tasks/build --config Release --target all -- -j 10
# indexing
RUN apt-get -y install bwa
# parition
# RUN apt-get -y install python3
# ADD python-util python-util
ENV PATH="/sgx-tasks/build/applications/genomeseq/dida-sgx/sample/DIDA:${PATH}"
EXPOSE 5050 5000


