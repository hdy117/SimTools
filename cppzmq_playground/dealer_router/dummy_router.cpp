#include <string>

#include "sync_rpc.h"
#include "sim_log.h"
#include "zmq.hpp"

#include "message/location.pb.h"
#include "message/basic.pb.h"
#include "message/trajectory.pb.h"


#if defined(_WIN32)
#pragma comment(lib, "libzmq-mt-4_3_5.lib")
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "glog.lib")
#pragma comment(lib, "gflags.lib")
#endif

int main() {
  // context with 8 io threads
  zmq::context_t context(2);

  // socket type of router
  zmq::socket_t router(context, zmq::socket_type::router);

  // bind server to port 5555 from any ip
  router.bind("tcp://0.0.0.0:5555");
  LOG_0 << "binding on tcp://0.0.0.0:5555.\n";

  // prepare message
  sim::Location location;
  location.mutable_t()->set_time_second(1.1);
  location.mutable_position()->set_x(1.2);
  location.mutable_position()->set_y(1.3);
  location.mutable_position()->set_z(1.4);

  sim::Trajectory trajectory;
  trajectory.mutable_t()->set_time_second(1.1);
  auto point = trajectory.add_points();
  point->mutable_position()->set_x(1.2);
  point->mutable_position()->set_y(1.3);
  point->mutable_position()->set_z(1.4);

  point = trajectory.add_points();
  point->mutable_position()->set_x(1.5);
  point->mutable_position()->set_y(1.6);
  point->mutable_position()->set_z(1.7);
  LOG_0 << "trajectory point_size:" << trajectory.points_size() << ", byte size:" << trajectory.ByteSizeLong() << ".\n";

  while (true) {
    zmq::message_t identityMsg, topicMsg;

    // request will automaticly insert id which router can see
    auto recv_result = router.recv(identityMsg, zmq::recv_flags::none);
    recv_result = router.recv(topicMsg, zmq::recv_flags::none);

    const std::string identity(static_cast<const char*>(identityMsg.data()));
    const std::string topic(static_cast<const char*>(topicMsg.data()));

    LOG_0 << "get topic:" << topic << ", size:" << topic.size() << " from:" << identity << "\n";

    if (topic == topic::LOCATION) {
      std::string payload;
      location.SerializeToString(&payload);

      LOG_0 << "reply with payload for topic:" << topic << ", payload size:" << payload.size() << "\n";

      // insert id and empty delmiter which router can see, so that router can
      // do the right routing
      zmq::message_t reply(payload.size() + 1);
      memcpy(reply.data(), payload.c_str(), payload.size() + 1);
      
      // use sendmore flag to make sure these three msg are sent as one
      auto send_result = router.send(identityMsg, zmq::send_flags::sndmore);
      send_result = router.send(reply, zmq::send_flags::none);
    } else if (topic == topic::TRAJECTORY) {
      std::string payloadTraj;
      trajectory.SerializeToString(&payloadTraj);

      LOG_0 << "reply with payload for topic:" << topic << ", payload size:" << payloadTraj.size() << "\n";

      zmq::message_t msgPayload(payloadTraj.size() + 1);
      memcpy(msgPayload.data(), payloadTraj.c_str(), payloadTraj.size() + 1);
      router.send(identityMsg, zmq::send_flags::sndmore);
      router.send(msgPayload, zmq::send_flags::none);
    }
    else {
      LOG_ERROR << "reply with payload for topic:" << topic << " is not implemented yet.\n";
    }
  }

  router.close();
  context.close();

  return 0;
}