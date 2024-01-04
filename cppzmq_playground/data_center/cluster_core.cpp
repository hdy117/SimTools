#include "cluster_core.h"

/////////////////////////////////////////////

bool MessageHelper::stringToZMQMsg(zmq::message_t& msg,
	const std::string& payload) {
	// payload.size() + 1 is important
	zmq::message_t tmpMsg(payload.size() + 1);
	memcpy(tmpMsg.data(), payload.c_str(), payload.size() + 1);
	msg = std::move(tmpMsg);
	return true;
}

bool MessageHelper::ZMQMsgToString(const zmq::message_t& msg,
	std::string& str) {
	// msg.size() - 1 is important
	str.clear();
	str = std::string(static_cast<const char*>(msg.data()), msg.size() - 1);
	return true;
}

/////////////////////////////////////////////
std::random_device MiscHelper::randDevice_;
std::mt19937 MiscHelper::generator_(MiscHelper::randDevice_());
std::uniform_int_distribution<> MiscHelper::distri_ = std::uniform_int_distribution<>(0, 10000);

MiscHelper::MiscHelper() {
}
MiscHelper::~MiscHelper() {}

int MiscHelper::randomInt() {
	return distri_(generator_);
}
float MiscHelper::randomFloat() {
	return randomInt() / 10000.0f;
}
