#include "glog/logging.h"
#include "grpc++/grpc++.h"

#include "msg/HelloService.grpc.pb.h"

#include <memory>
#include <string>

class GreeterServiceImpl final : public sim::Greeter::Service {
  grpc::Status SayHello(grpc::ServerContext *context,
                        const sim::HelloRequest *request,
                        sim::HelloResponse *reply) override {

    // LOG(INFO) << "request name is " << request->name() << "\n";
    reply->set_replay("Hi, " + request->name());
    return grpc::Status::OK;
  }
};

int main() {
  std::string server_address("0.0.0.0:5556");
  GreeterServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  LOG(INFO) << "Server listening on: " << server_address << "\n";

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  return 0;
}