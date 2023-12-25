#pragma once

#include "sim.h"

#include "message/location.pb.h"
#include "message/basic.pb.h"
#include "message/trajectory.pb.h"
#include "message/rpc.pb.h"
#include "zmq.hpp"

#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <future>
#include <functional>
#include <unordered_map>

#define FATAL_STATE 2

namespace topic {
const std::string LOCATION("LOCATION");
const std::string TRAJECTORY("TRAJECTORY");
} // namespace topic

class InternalState {
public:
	InternalState():state_(0), info_("OK") {}
	virtual ~InternalState() {}
	void setState(uint32_t state, const std::string& info="OK") {
		std::lock_guard<std::mutex> guard(lock_);
		state_ = state;
		info_ = info;
	}
	uint32_t getState() {
		std::lock_guard<std::mutex> guard(lock_);
		return state_;
	}
	std::string getStateInfo() {
		std::lock_guard<std::mutex> guard(lock_);
		return 	info_;
		;
	}
private:
	uint32_t state_;
	std::string info_;
	std::mutex lock_;
};

class MessageHelper {
public:
	static void printMessage(const std::string& topic, const std::string& payload);
	static void printMessage(const std::string& topic, const google::protobuf::Message& message);
	static bool parseFromMessage(const zmq::message_t& msg, google::protobuf::Message& message);
};

/**
 * @brief sync_rpc client, 
 * send data format: id, RPCCallInfo, request_payload(protobuf or string, service should know how to deal)
 * reply data format: id, RPCCallInfo, RPCServiceStatus(if 2, no reply), reply_payload(protobuf or string, client should know how to deal) 
*/
class Client {
public:
	Client(const std::string& serverIP, const std::string& port, const std::string& clientID);
	virtual ~Client();

	Client(const Client&) = delete;
	Client& operator=(const Client&) = delete;
public:
	//sim::RPCServiceStatus registerWithServer();
	sim::RPCServiceStatus getMessageByTopic(const std::string& topic, std::string& payload);
	sim::RPCServiceStatus setMessageByTopic(const std::string& topic, const std::string& payload);
private:
	//sim::RPCServiceStatus sendHeartBeat();
private:
	std::string id_;
	std::string serverIP_, serverPort_, serverAddr_;
	std::atomic<uint8_t> stop_;
	InternalState internalState_;			// state of client, 0 means ok, 1 means warning, 2 means fatal

	zmq::context_t context_;
	zmq::socket_t rpcSocket_;
};

// service 
class ServiceImp;
using ServiceImpPtr = std::shared_ptr<ServiceImp>;

class Server {
public:
	Server(const std::string& serverIP, const std::string& port, const std::string& clientID);
	virtual ~Server();

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;
public:
	void serve();
protected:
	void serveGetMessageByTopic();
private:
	ServiceImpPtr service_;
	std::string serverIP_, serverPort_, serverAddr_;
	zmq::context_t context_;
};

class ServiceImp {
public:
	ServiceImp();
	virtual ~ServiceImp();

	ServiceImp(const ServiceImp&) = delete;
	ServiceImp& operator=(const ServiceImp&) = delete;

	sim::RPCServiceStatus getMessageByTopic(const std::string& topic, std::string& payload);
	sim::RPCServiceStatus setMessageByTopic(const std::string& topic, const std::string& payload);
private:
	
};