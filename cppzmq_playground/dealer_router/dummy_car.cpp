#include "sim_log.h"
#include "zmq.hpp"

#include "sync_rpc.h"

#include <chrono>
#include <string>

int main() {
	Client client("127.0.0.1", "5555", names::PerfectControl);

	sim::Location location;
	sim::Trajectory trajectory;

	for (auto i = 0; i < 100; ++i) {
		LOG_0 << "=====================\n";
		std::string payload;

		// subscribe
		client.getMessageByTopic(topic::TRAJECTORY, payload);
		MessageHelper::printMessage(topic::TRAJECTORY, payload);

		// data
		location.mutable_t()->set_time_second(i / 1000.0);

		// publish
		location.SerializeToString(&payload);
		client.setMessageByTopic(topic::LOCATION, payload);
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
  //zmq::context_t context(1);
  //zmq::socket_t socket(context, zmq::socket_type::dealer);

  //// set identity with PerfectControl, dealer has to have an identity, so that router will do right message routing. 
  //// If dealer do not have an identity, router will add one randomly uuid to this connection
  //socket.setsockopt(ZMQ_IDENTITY, names::PerfectControl.c_str(),
  //  names::PerfectControl.size()+1);
  //socket.connect("tcp://127.0.0.1:5555"); // 172.18.224.1 is server ip

  //// set topic to get from router
  //zmq::message_t request(topic::TRAJECTORY.size() + 1);
  //memcpy(request.data(), topic::TRAJECTORY.c_str(), topic::TRAJECTORY.size() + 1);
  //auto send_result = socket.send(request, zmq::send_flags::none);

  //// wait for reply
  //zmq::message_t reply;
  //auto recv_result = socket.recv(reply, zmq::recv_flags::none);

  //// parse payload, size is important in case of reply string contains '\0'
  //std::string payload(static_cast<const char*>(reply.data()), reply.size() - 1);
  //sim::Trajectory trajectory;
  //trajectory.ParseFromString(payload);
  //LOG_0 << "payload.size():" << payload.size() << ", trajectory:" << trajectory.DebugString() << "\n";

  //socket.close();
  //context.close();

  return 0;
}