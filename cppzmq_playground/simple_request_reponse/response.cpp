#include <string>

#include "zmq.hpp"
#include "sim_log.h"

int main() { 
	// context with 8 io threads
	zmq::context_t context(8);

	// socket type of response/replay
	zmq::socket_t socket(context, zmq::socket_type::rep);

	// bind server to port 5555 from any ip
	socket.bind("tcp://0.0.0.0:5555");

	while (true) {
		zmq::message_t request_msg;

		LOG_0 << "server waiting request msg.\n";
		auto recv_result = socket.recv(request_msg, zmq::recv_flags::none);
		LOG_0 << "server recv result:" << recv_result.value() << ".\n";
		LOG_0 << "server received msg:" << static_cast<char*>(request_msg.data()) << ", size:" << request_msg.size() << ".\n";

		zmq::message_t replay(6);
		memcpy(replay.data(), "world", 6);
		auto send_result = socket.send(replay, zmq::send_flags::none);
		LOG_0 << "server send result:" << send_result.value() << ".\n";
	}

	socket.close();

	return 0; 
}