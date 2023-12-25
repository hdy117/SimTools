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

bool MessageHelper::parseFromZMQMsg(const zmq::message_t& msg, google::protobuf::Message& message) {
	std::string payload;
	MessageHelper::ZMQMsgToString(msg, payload);
	message.ParseFromString(payload);
	return true;
}

bool MessageHelper::protobufToZMQMsg(zmq::message_t& msg, const google::protobuf::Message& message) {
	std::string payload;
	message.SerializeToString(&payload);
	return MessageHelper::stringToZMQMsg(msg, payload);
}

bool MessageHelper::stringToZMQMsg(zmq::message_t& msg, const std::string& payload) {
	// payload.size() + 1 is important
	zmq::message_t tmpMsg(payload.size() + 1);
	memcpy(tmpMsg.data(), payload.c_str(), payload.size() + 1);
	msg = std::move(tmpMsg);
	return true;
}

bool MessageHelper::ZMQMsgToString(const zmq::message_t& msg, std::string& str) {
	// msg.size() - 1 is important
	str.clear();
	str = std::string(static_cast<const char*>(msg.data()), msg.size() - 1);
	return true;
}

/*============================================================*/

Client::Client(const std::string& serverIP, const std::string& port, const std::string& clientID) {
	serverIP_ = serverIP;
	serverPort_ = port;
	id_ = clientID;
	serverAddr_ = "tcp://" + serverIP_ + ":" + serverPort_;
	LOG_0 << "connect to " << serverAddr_ << "\n";

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

	// prepare msg to send
	zmq::message_t callInfoMsg, topicMsg;
	MessageHelper::protobufToZMQMsg(callInfoMsg, callInfo);
	MessageHelper::stringToZMQMsg(topicMsg, topic);

	// send rpc call
	rpcSocket_.send(callInfoMsg, zmq::send_flags::sndmore);
	rpcSocket_.send(topicMsg, zmq::send_flags::none);

	// recv rpc response
	zmq::message_t servStateMsg;
	rpcSocket_.recv(callInfoMsg, zmq::recv_flags::none);
	rpcSocket_.recv(servStateMsg, zmq::recv_flags::none);
	MessageHelper::parseFromZMQMsg(servStateMsg, serviceStatus);

	// if serv is fatal
	if (serviceStatus.state() == FATAL_STATE) {
		return serviceStatus;
	}
		
	// recv
	zmq::message_t replyMsg;
	rpcSocket_.recv(replyMsg, zmq::recv_flags::none);
	
	// get payload
	MessageHelper::ZMQMsgToString(replyMsg, payload);
	
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
	sim::RPCMsgPair msgPair;
	msgPair.set_topic(topic.c_str());
	msgPair.set_payload(payload.c_str(), payload.size());
	
	std::string payloadPair;
	msgPair.SerializeToString(&payloadPair);
	
	zmq::message_t callInfoMsg(callInfoPayload.size() + 1), pairMsg(payloadPair.size() + 1);
	MessageHelper::protobufToZMQMsg(callInfoMsg, callInfo);
	MessageHelper::protobufToZMQMsg(pairMsg, msgPair);

	// send rpc call
	rpcSocket_.send(callInfoMsg, zmq::send_flags::sndmore);
	rpcSocket_.send(pairMsg, zmq::send_flags::none);

	// recv rpc response
	zmq::message_t servStateMsg;
	rpcSocket_.recv(callInfoMsg, zmq::recv_flags::none);
	rpcSocket_.recv(servStateMsg, zmq::recv_flags::none);
	MessageHelper::parseFromZMQMsg(servStateMsg, serviceStatus);

	// if serv is fatal
	if (serviceStatus.state() == FATAL_STATE) {
		return serviceStatus;
	}

	// recv
	zmq::message_t replyMsg;
	rpcSocket_.recv(replyMsg, zmq::recv_flags::none);

	return serviceStatus;
}

/*==========================================*/

Server::Server(const std::string& serverIP, const std::string& port) {
	stop_ = false;
	serverIP_ = serverIP;
	serverPort_ = port;
	serverAddr_ = "tcp://" + serverIP_ + ":" + serverPort_;
	LOG_0 << "binding on " << serverAddr_ << "\n";

	context_ = zmq::context_t(1);
	rpcSocket_ = zmq::socket_t(context_, zmq::socket_type::router);
	try {
		rpcSocket_.bind(serverAddr_.c_str());
	}
	catch (const std::exception& e) {
		internalState_.setState(2, "fail to connect server");
		LOG_ERROR << "error:" << e.what() << "\n";
		throw std::runtime_error("fail to bind server on specific port");
	}
}
Server::~Server() {
	rpcSocket_.close();
	context_.close();
}

void Server::serve() {
	service_ = std::make_shared<ServiceImp_A>();

	if (service_.get() == nullptr) {
		internalState_.setState(2, "fail to build service");
		throw std::runtime_error("fail to build service");
		return;
	}

	while (!stop_) {
		// recv rpc call from dealer
		zmq::message_t msgID, msgCallInfo, msgReq;
		rpcSocket_.recv(msgID, zmq::recv_flags::none);
		rpcSocket_.recv(msgCallInfo, zmq::recv_flags::none);
		rpcSocket_.recv(msgReq, zmq::recv_flags::none);

		// parse message
		sim::RPCCallInfo callInfo;
		MessageHelper::parseFromZMQMsg(msgCallInfo, callInfo);

		// serve
		zmq::message_t msgReply, servStatusMsg;
		auto servStatus = service_->dispatch(callInfo, msgReq, msgReply);
		
		// return result
		MessageHelper::protobufToZMQMsg(servStatusMsg, servStatus);
		rpcSocket_.send(msgID, zmq::send_flags::sndmore);
		rpcSocket_.send(msgCallInfo, zmq::send_flags::sndmore);
		rpcSocket_.send(servStatusMsg, zmq::send_flags::sndmore);
		rpcSocket_.send(msgReply, zmq::send_flags::none);
	}
}

sim::RPCServiceStatus ServiceImpBase::dispatch(const sim::RPCCallInfo& callInfo, const zmq::message_t& msgReq, zmq::message_t& msgReply) {
	sim::RPCServiceStatus serviceStatus;

	// ugly but let's use this as a demo
	if (callInfo.funcname() == std::string("setMessageByTopic_string_string")) {
		sim::RPCMsgPair msgPair;
		MessageHelper::parseFromZMQMsg(msgReq, msgPair);
		setMessageByTopic(msgPair.topic(), msgPair.payload());
	}
	else if (callInfo.funcname() == std::string("getMessageByTopic_string_string")) {
		std::string topic, payload;
		MessageHelper::ZMQMsgToString(msgReq, topic);
		getMessageByTopic(topic, payload);
		MessageHelper::stringToZMQMsg(msgReply, payload);
	}
	else {
		LOG_ERROR << "unknown funcName:" << callInfo.funcname() << "\n";
	}

	return serviceStatus;
}

ServiceImp_A::ServiceImp_A() { 
	msgMap_.clear(); 
	LOG_0 << "ServiceImp_A constructed.\n";
}
ServiceImp_A::~ServiceImp_A() { msgMap_.clear(); }

sim::RPCServiceStatus ServiceImp_A::getMessageByTopic(const std::string& topic, std::string& payload) {
	sim::RPCServiceStatus serviceStatus;
	auto iter = msgMap_.find(topic);
	payload = iter == msgMap_.end() ? std::string() : iter->second;
	return serviceStatus;
}
sim::RPCServiceStatus ServiceImp_A::setMessageByTopic(const std::string& topic, const std::string& payload) {
	sim::RPCServiceStatus serviceStatus;
	msgMap_[topic] = payload;
	return serviceStatus;
}
