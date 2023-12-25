#include "sync_rpc.h"
#include "sim_log.h"

void MessageHelper::printMessage(const std::string& topic, const std::string& payload) {
	if (topic::LOCATION == topic) {
		sim::Location location;
		location.ParseFromString(payload);
		MessageHelper::printMessage(topic, location);
	}
	else if (topic::TRAJECTORY == topic) {
		sim::Trajectory trajectory;
		trajectory.ParseFromString(payload);
		MessageHelper::printMessage(topic, trajectory);
	}
	else {
		LOG_ERROR << "unknown topic " << topic << "\n";
	}
}

void MessageHelper::printMessage(const std::string& topic, const google::protobuf::Message& message) {
	LOG_0 << "topic:" << topic << ", payload:" << message.DebugString() << "\n";
}

bool MessageHelper::parseFromMessage(const zmq::message_t& msg, google::protobuf::Message& message) {
	message.ParseFromArray(msg.data(), msg.size() - 1);
	return true;
}

/*============================================================*/

Client::Client(const std::string& serverIP, const std::string& port, const std::string& clientID) {
	serverIP_ = serverIP;
	serverPort_ = port;
	id_ = clientID;
	serverAddr_ = "tcp://" + serverIP_ + ":" + serverPort_;
	LOG_0 << "serverAddr_:" << serverAddr_ << "\n";

	context_ = zmq::context_t(1);
	rpcSocket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	if (id_.empty()) {
		LOG_ERROR << "client id should be unique, current is " << clientID << "\n";
		throw std::runtime_error("client id should be unique(not empty)");
	}
	rpcSocket_.setsockopt(ZMQ_IDENTITY, id_.c_str(), id_.size() + 1);
	try {
		rpcSocket_.connect(serverAddr_.c_str());
		LOG_0 << "client id is " << id_ << "\n";
	}
	catch (const std::exception& e) {
		internalState_.setState(1, "fail to connect server");
		LOG_ERROR << "error:" << e.what() << "\n";
		throw std::runtime_error("fail to connect server");
	}
}
Client::~Client() {
	rpcSocket_.close();
	context_.close();
}
sim::RPCServiceStatus Client::getMessageByTopic(const std::string& topic, std::string& payload) {
	// clear firstly
	payload.clear();

	sim::RPCServiceStatus serviceStatus;
	if (topic.empty()) {
		serviceStatus.set_state(1);
		serviceStatus.set_info("empty topic will not be served");
		return serviceStatus;
	}

	// rpc call info
	sim::RPCCallInfo callInfo;
	callInfo.set_funcname("getMessageByTopic_string_string");
	callInfo.set_flag(0);
	std::string callInfoPayload;
	callInfo.SerializeToString(&callInfoPayload);

	// prepare msg to send
	zmq::message_t callInfoMsg(callInfoPayload.size() + 1), topicMsg(topic.size() + 1);

	// send rpc call
	rpcSocket_.send(callInfoMsg, zmq::send_flags::sndmore);
	rpcSocket_.send(topicMsg, zmq::send_flags::none);

	// recv rpc response
	zmq::message_t servStateMsg;
	rpcSocket_.recv(servStateMsg, zmq::recv_flags::none);
	MessageHelper::parseFromMessage(servStateMsg, serviceStatus);

	// if serv is fatal
	if (serviceStatus.state() == FATAL_STATE) {
		return serviceStatus;
	}
		
	// recv
	zmq::message_t replyMsg;
	rpcSocket_.recv(replyMsg, zmq::recv_flags::none);
	
	// copy data out
	MessageHelper::parseFromMessage(servStateMsg, serviceStatus);
	
	// post work
	MessageHelper::printMessage(topic, payload);
	
	return serviceStatus;
}

sim::RPCServiceStatus Client::setMessageByTopic(const std::string& topic, const std::string& payload) {
	sim::RPCServiceStatus serviceStatus;
	if (topic.empty()) {
		serviceStatus.set_state(1);
		serviceStatus.set_info("empty topic will not be served");
		return serviceStatus;
	}

	// rpc call info
	sim::RPCCallInfo callInfo;
	callInfo.set_funcname("setMessageByTopic_string_string");
	callInfo.set_flag(0);
	std::string callInfoPayload;
	callInfo.SerializeToString(&callInfoPayload);

	// prepare msg to send
	sim::MsgPair pair;
	pair.set_topic(topic.c_str());
	pair.set_payload(payload.c_str(), pair.size());
	std::string payloadPair;
	pair.SerializeToString(&payloadPair);
	zmq::message_t callInfoMsg(callInfoPayload.size() + 1), pairMsg(payloadPair.size() + 1);

	// send rpc call
	rpcSocket_.send(callInfoMsg, zmq::send_flags::sndmore);
	rpcSocket_.send(pairMsg, zmq::send_flags::none);

	// recv rpc response
	zmq::message_t servStateMsg;
	rpcSocket_.recv(servStateMsg, zmq::recv_flags::none);
	MessageHelper::parseFromMessage(servStateMsg, serviceStatus);

	// if serv is fatal
	if (serviceStatus.state() == FATAL_STATE) {
		return serviceStatus;
	}

	// recv
	zmq::message_t replyMsg;
	rpcSocket_.recv(replyMsg, zmq::recv_flags::none);

	// copy data out
	MessageHelper::parseFromMessage(servStateMsg, serviceStatus);

	// post work
	MessageHelper::printMessage(topic, payload);

	return serviceStatus;
}
