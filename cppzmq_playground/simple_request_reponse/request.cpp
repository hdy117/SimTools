#include "zmq.hpp"
#include "sim_log.h"

#include <string>

int main() { 
	zmq::context_t context(1);
	zmq::socket_t socket(context, zmq::socket_type::req);
	socket.connect("tcp://172.18.224.1:5555"); // 172.18.224.1 is server ip

	zmq::message_t request(6);
	memcpy(request.data(), "hello", 6);

	zmq::send_result_t send_result = socket.send(request, zmq::send_flags::none);
	LOG_0 << "client send result:" << send_result.value() << ".\n";
	LOG_0 << "client send msg:" << static_cast<const char*>(request.data()) << ".\n";

	zmq::message_t replay;
	zmq::recv_result_t recv_result = socket.recv(replay, zmq::recv_flags::none);
	LOG_0 << "client recv result:" << recv_result.value() << ".\n";
	LOG_0 << "client received msg:" << static_cast<const char*>(replay.data()) << ".\n";

	socket.close();

	return 0; 
}