FROM ubuntu:22.04

# 0. work dir
WORKDIR /root/tmp
COPY ./deps/*.zip /root/tmp/

# 1. update os
RUN apt update && apt upgrade -y && apt install -y apt-utils tzdata
ENV TZ="Asia/ShangHai"
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 2. install necessary libraries and tools
RUN apt install -y python3-pip libeigen3-dev libjsoncpp-dev gcc cmake build-essential \
    libgoogle-glog-dev libgrpc-dev libbullet-dev libbullet-extras-dev libbullet-doc git unzip && apt clean
RUN unzip -x libzmq-4.3.5.zip && cd libzmq-4.3.5 && \
    mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && make install
RUN unzip -x cppzmq-4.10.0.zip && cd cppzmq-4.10.0  && \
    mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && make install

# 3. install python packages
RUN python3 -m pip install django zmq glog grpcio-tools -i https://pypi.douban.com/simple

# 4. more libraries and tools
RUN apt install -y libprotobuf-dev protobuf-compiler net-tools iputils-ping vim \
    protobuf-compiler-grpc libgrpc++-dev && \
    apt clean

# end. ld-config
RUN rm -rf /root/tmp/
WORKDIR /root/sim
RUN ldconfig