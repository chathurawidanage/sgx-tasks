FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
    gnupg \
    wget

RUN echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu bionic main' > /etc/apt/sources.list.d/intel-sgx.list
RUN wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add -
RUN apt-get update 
RUN apt-get install -y build-essential
WORKDIR /opt/intel
RUN wget https://download.01.org/intel-sgx/sgx-linux/2.8/distro/ubuntu18.04-server/sgx_linux_x64_sdk_2.8.100.3.bin
RUN chmod +x sgx_linux_x64_sdk_2.8.100.3.bin
RUN echo 'yes' | ./sgx_linux_x64_sdk_2.8.100.3.bin

RUN wget http://archive.ubuntu.com/ubuntu/pool/main/p/protobuf/libprotobuf10_3.0.0-9.1ubuntu1_amd64.deb
RUN dpkg -i libprotobuf10_3.0.0-9.1ubuntu1_amd64.deb
RUN apt-get install -y \
    libcurl4 \
    libssl1.1

RUN apt-get install -y --no-install-recommends libsgx-launch libsgx-urts
RUN apt-get install -y \
    libcurl4 \
    libssl1.1 \
    make 
RUN apt-get install -y libsgx-aesm-launch-plugin