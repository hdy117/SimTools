#include "msg/person.pb.h"
#include "msg_gen.h"

MsgGen::MsgGen() {}
MsgGen::~MsgGen() {}

const std::string &MsgGen::getPayload() const { return payload_; }
void MsgGen::genMsgPayload() {
  sim::Person person;
  person.set_name("Jim");
  person.set_age(25);
  person.SerializeToString(&payload_);
}