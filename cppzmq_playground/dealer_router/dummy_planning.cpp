#include "sim_log.h"
#include "zmq.hpp"

#include "sync_rpc.h"

#include <chrono>
#include <string>

int main() {
  zmq::context_t context(1);
  zmq::socket_t socket(context, zmq::socket_type::dealer);

  // set identity with PLANNING
  std::string addr("tcp://127.0.0.1:5555");
  socket.setsockopt(ZMQ_IDENTITY, names::PLANNING.c_str(), names::PLANNING.size()+1);
  socket.connect(addr); // 172.18.224.1 is server ip
  LOG_0 << "connecting "<< addr << "\n";

  // set topic to get from router
  zmq::message_t request(topic::LOCATION.size() + 1), delimiterMsg(0);
  memcpy(request.data(), topic::LOCATION.c_str(), topic::LOCATION.size() + 1);
  /*zmq::send_result_t send_result = socket.send(delimiterMsg, zmq::send_flags::sndmore);*/
  auto send_result = socket.send(request, zmq::send_flags::none);
  LOG_0 << "send location request state " << static_cast<size_t>(send_result.value()) << "\n";

  // wait for reply
  zmq::message_t reply;
  //zmq::recv_result_t recv_result = socket.recv(delimiterMsg, zmq::recv_flags::none);
  auto recv_result = socket.recv(reply, zmq::recv_flags::none);

  // parse payload
  std::string payload(static_cast<const char *>(reply.data()));
  sim::Location location;
  location.ParseFromString(payload);
  LOG_0 << "location:" << location.DebugString() << "\n";

  socket.close();
  context.close();

  return 0;
}