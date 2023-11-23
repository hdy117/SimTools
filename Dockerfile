FROM ubuntu:22.04

# 1. update os
RUN apt update && apt upgrade -y

# 2. install necessary libraries and tools
RUN apt install -y python3-pip libeigen3-dev libjsoncpp-dev gcc cmake build-essential

# 3. install python packages
RUN python3 -m pip install django zmq glog grpcio-tools -i https://pypi.douban.com/simple

# 4. ld-config
RUN ld-config