#include "sim_log.h"
#include "zmq.hpp"

#include "sync_rpc.h"

#include <chrono>
#include <string>

int main() {
  zmq::context_t context(1);
  zmq::socket_t socket(context, zmq::socket_type::dealer);

  // set identity with PLANNING, dealer has to have an identity, so that router will do right message routing. 
  // If dealer do not have an identity, router will add one randomly uuid to this connection
  std::string addr("tcp://127.0.0.1:5555");
  socket.setsockopt(ZMQ_IDENTITY, names::PLANNING.c_str(), names::PLANNING.size()+1);
  socket.connect(addr); // 172.18.224.1 is server ip
  LOG_0 << "connecting "<< addr << "\n";

  // set topic to get from router
  zmq::message_t request(topic::LOCATION.size() + 1);
  memcpy(request.data(), topic::LOCATION.c_str(), topic::LOCATION.size() + 1);
  auto send_result = socket.send(request, zmq::send_flags::none);
  LOG_0 << "send location request state " << static_cast<size_t>(send_result.value()) << "\n";

  // wait for reply
  zmq::message_t reply;
  auto recv_result = socket.recv(reply, zmq::recv_flags::none);

  // parse payload, size is important in case of reply string contains '\0'
  std::string payload(static_cast<const char *>(reply.data()), reply.size() - 1);
  sim::Location location;
  location.ParseFromString(payload);
  LOG_0 << "location:" << location.DebugString() << "\n";

  socket.close();
  context.close();

  return 0;
}