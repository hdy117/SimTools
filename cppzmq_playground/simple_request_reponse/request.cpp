#include "zmq.hpp"
#include "sim_log.h"

#include <string>
#include <chrono>

int main() { 
	zmq::context_t context(1);
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://127.0.0.1:5555"); // 172.18.224.1 is server ip

	std::string name = "";
    const size_t SIZE_1K = 1024;
    for (auto i = 0; i < SIZE_1K; ++i) {
      name += std::to_string(i);
    }
	auto t1 = std::chrono::high_resolution_clock::now();
	const size_t LOOPS = 100000;
    for (auto i = 0; i < LOOPS; ++i) {
		zmq::message_t request(name.size());
		memcpy(request.data(), name.c_str(), name.size());

		zmq::send_result_t send_result = socket.send(request, zmq::send_flags::none);
		
		zmq::message_t replay;
		zmq::recv_result_t recv_result = socket.recv(replay, zmq::recv_flags::none);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() /
        1000.0;
    LOG_0 << LOOPS << " sayhello cost[ms]:" << duration
              << ", which means[msg/ms]:" << LOOPS / duration
              << ", msg size[byte]:" << SIZE_1K << "\n";

	socket.close();

	return 0; 
}