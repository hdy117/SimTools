#include "sim_log.h"
#include "zmq.hpp"

#include "sync_grpc.h"

#include <chrono>
#include <string>

int main() {
  zmq::context_t context(1);
  zmq::socket_t socket(context, zmq::socket_type::req);

  // set identity with PLANNING
  socket.setsockopt(ZMQ_IDENTITY, names::PLANNING.c_str(),
    names::PLANNING.size());
  socket.connect("tcp://127.0.0.1:5555"); // 172.18.224.1 is server ip

  // set topic to get from router
  zmq::message_t request(topic::LOCATION.size() + 1);
  memcpy(request.data(), topic::LOCATION.c_str(), topic::LOCATION.size() + 1);
  zmq::send_result_t send_result = socket.send(request, zmq::send_flags::none);

  // wait for reply
  zmq::message_t reply;
  zmq::recv_result_t recv_result = socket.recv(reply, zmq::recv_flags::none);

  // parse payload
  std::string payload(static_cast<const char *>(reply.data()));
  sim::Location location;
  location.ParseFromString(payload);
  LOG_0 << "location:" << location.DebugString() << "\n";

  socket.close();

  return 0;
}