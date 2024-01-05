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

void MessageHelper::swapBuffer(char* buffer1, char* buffer2, uint32_t maxSize) {
	char tmpBuffer[constant::kBufferSize_1024] = { '\0' };
	memcpy(tmpBuffer, buffer1, maxSize);
	memcpy(buffer1, buffer2, maxSize);
	memcpy(buffer2, tmpBuffer, maxSize);
}
void MessageHelper::copyStringToBuffer(char* buffer, const std::string& str, uint32_t maxBufferSize) {
	memcpy(buffer, str.c_str(), str.size() + 1); 
	buffer[maxBufferSize - 1] = '\0';
}

/////////////////////////////////////////////
std::random_device MiscHelper::randDevice_;
std::mt19937 MiscHelper::generator_(MiscHelper::randDevice_());
std::uniform_int_distribution<> MiscHelper::distri_ = std::uniform_int_distribution<>(0, 100);

MiscHelper::MiscHelper() {
}
MiscHelper::~MiscHelper() {}

int MiscHelper::randomInt() {
	return distri_(generator_);
}
float MiscHelper::randomFloat() {
	return randomInt() / 100.0f;
}
