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

////////////////////////////////

ClusterStateProxy::ClusterStateProxy(const std::string& xpubPort, const std::string& xsubPort) {
	std::string xpubAddr = "tcp://0.0.0.0:" + xpubPort;
	std::string xsubAddr = "tcp://0.0.0.0:" + xsubPort;

	context_ = zmq::context_t(1);
	socketXPub_ = zmq::socket_t(context_, zmq::socket_type::xpub); 
	socketXSub_ = zmq::socket_t(context_, zmq::socket_type::xsub);

	socketXPub_.bind(xpubAddr);
	socketXSub_.bind(xsubAddr);
}
ClusterStateProxy::~ClusterStateProxy() {
	socketXPub_.close();
	socketXSub_.close();
	context_.close();
}

void ClusterStateProxy::runTask() {
	LOG_0 << "hi, this is cluster state proxy thread.\n";
	// blocking proxy
	zmq::proxy(socketXSub_, socketXPub_, nullptr);
}