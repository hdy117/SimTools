#include "sim_log.h"
#include "zmq.hpp"

#include "sync_rpc.h"

#include <chrono>
#include <string>

int main() {
	Client client("127.0.0.1", "5555", names::PLANNING);

	sim::Location location;
	sim::Trajectory trajectory;

	for (auto i = 0; i < 100; ++i) {
		LOG_0 << "=====================\n";
		std::string payload;

		// subscribe
		client.getMessageByTopic(topic::LOCATION, payload);
		MessageHelper::printMessage(topic::LOCATION, payload);

		// data
		trajectory.mutable_t()->set_time_second(i / 100.0);
		auto point = trajectory.add_point();
		point->mutable_position()->set_x(i / 1.0);
		point->mutable_position()->set_y(i / 2.0);
		point->mutable_position()->set_z(i / 4.0);

		// publish
		trajectory.SerializeToString(&payload);
		client.setMessageByTopic(topic::TRAJECTORY, payload);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

  //zmq::context_t context(1);
  //zmq::socket_t socket(context, zmq::socket_type::dealer);

  //// set identity with PLANNING, dealer has to have an identity, so that router will do right message routing. 
  //// If dealer do not have an identity, router will add one randomly uuid to this connection
  //std::string addr("tcp://127.0.0.1:5555");
  //socket.setsockopt(ZMQ_IDENTITY, names::PLANNING.c_str(), names::PLANNING.size()+1);
  //socket.connect(addr); // 172.18.224.1 is server ip
  //LOG_0 << "connecting "<< addr << "\n";

  //// set topic to get from router
  //zmq::message_t request(topic::LOCATION.size() + 1);
  //memcpy(request.data(), topic::LOCATION.c_str(), topic::LOCATION.size() + 1);
  //auto send_result = socket.send(request, zmq::send_flags::none);
  //LOG_0 << "send location request state " << static_cast<size_t>(send_result.value()) << "\n";

  //// wait for reply
  //zmq::message_t reply;
  //auto recv_result = socket.recv(reply, zmq::recv_flags::none);

  //// parse payload, size is important in case of reply string contains '\0'
  //std::string payload(static_cast<const char *>(reply.data()), reply.size() - 1);
  //sim::Location location;
  //location.ParseFromString(payload);
  //LOG_0 << "location:" << location.DebugString() << "\n";

  //socket.close();
  //context.close();

  return 0;
}