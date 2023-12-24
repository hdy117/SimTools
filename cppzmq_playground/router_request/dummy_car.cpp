#include "sim_log.h"
#include "zmq.hpp"

#include "sync_grpc.h"

#include <chrono>
#include <string>

int main() {
  zmq::context_t context(1);
  zmq::socket_t socket(context, zmq::socket_type::req);

  // set identity with PerfectControl
  socket.setsockopt(ZMQ_IDENTITY, names::PerfectControl.c_str(),
    names::PerfectControl.size());
  socket.connect("tcp://127.0.0.1:5555"); // 172.18.224.1 is server ip

  // set topic to get from router
  zmq::message_t request(topic::TRAJECTORY.size() + 1);
  memcpy(request.data(), topic::TRAJECTORY.c_str(), topic::TRAJECTORY.size() + 1);
  zmq::send_result_t send_result = socket.send(request, zmq::send_flags::none);

  // wait for reply
  zmq::message_t reply;
  zmq::recv_result_t recv_result = socket.recv(reply, zmq::recv_flags::none);

  // parse payload
  std::string payload(static_cast<const char*>(reply.data()));
  sim::Trajectory trajectory;
  trajectory.ParseFromString(payload);
  LOG_0 << "trajectory:" << trajectory.DebugString() << "\n";

  socket.close();

  return 0;
}