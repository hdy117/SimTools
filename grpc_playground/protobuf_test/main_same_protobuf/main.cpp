#include "gen_msg/msg_gen.h"
#include "msg/person.pb.h"

#include <iostream>
#include <string>

int main() {
  MsgGen msg_gen;

  msg_gen.genMsgPayload();
  const auto &payload = msg_gen.getPayload();

  sim::Person person;

  person.ParseFromString(payload);

  std::cout << "person.name():" << person.name() << "\n";
  std::cout << "person.age():" << person.age() << "\n";

  return 0;
}