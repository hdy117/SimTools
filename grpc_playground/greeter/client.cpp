#include "glog/logging.h"
#include "grpc++/grpc++.h"

#include "msg/HelloService.grpc.pb.h"

#include <chrono>
#include <string>

int main(int argc, char *argv[]) {
  if (argc == 3) {
    std::string ip = argv[1];
    std::string port = argv[2];
    std::string addr = ip + ":" + port;

    LOG(INFO) << "client --> " << addr << "\n";

    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<sim::Greeter::Stub> stub(sim::Greeter::NewStub(channel));

    sim::HelloRequest request_msg;
    sim::HelloResponse response_msg;
    std::string name = "";
    const size_t SIZE_1K = 1024;
    for (auto i = 0; i < SIZE_1K; ++i) {
      name += std::to_string(i);
    }
    LOG(INFO) << "name:" << name << ", " << name.size() << "\n";
    request_msg.set_name(name.c_str());

    auto t1 = std::chrono::high_resolution_clock::now();
    const size_t LOOPS = 100000;
    for (auto i = 0; i < LOOPS; ++i) {
      grpc::ClientContext context;
      grpc::Status status =
          stub->SayHello(&context, request_msg, &response_msg);

      if (!status.ok()) {
        LOG(ERROR) << "stub->SayHello status not ok.\n";
        break;
      }
    }
    LOG(INFO) << "stub->SayHello got " << response_msg.replay() << "\n";

    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() /
        1000.0;
    LOG(INFO) << LOOPS << " sayhello cost[ms]:" << duration
              << ", which means[msg/ms]:" << LOOPS / duration
              << ", msg size[byte]:" << SIZE_1K << "\n";
  }

  return 0;
}