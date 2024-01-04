#pragma once

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <queue>
#include <random>
#include <iomanip>
#include <memory>

#include "zmq.hpp"
#include "sim_log.h"

#if defined(_WIN32)
#pragma comment(lib, "libzmq-mt-4_3_5.lib")
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "glog.lib")
#pragma comment(lib, "gflags.lib")
#endif

struct TaskMeta {
	uint64_t taskID;
	uint64_t taskType;
};

struct Task {
	TaskMeta meta;
	const static size_t size = 1024;
	double arr[Task::size];
};

struct TaskResult {
	TaskMeta meta;
	double sum;
};

struct ClusterStateInfo {
	const static uint32_t kMaxNameSize = 64;
	char clusterName[kMaxNameSize] = {'\0'};
	uint32_t readyWorkerCount = 0;
};

class AsyncRun;
class ClusterStateProxy;
using AsyncRunPtr = std::shared_ptr<AsyncRun>;
using ClusterStateInfoPtr = std::shared_ptr<ClusterStateInfo>;
using ClusterStateProxyPtr = std::shared_ptr<ClusterStateProxy>;

namespace constant {
	// super broker IP
	const std::string superBroker_IP("127.0.0.1");

	// pull port to collect all cluster broker state in super broker
	const std::string kPullPort("5557");

	// local port of front and back end for client and worker in one cluster
	const std::string kLocal_Frontend("5558");
	const std::string kLocal_backend("5559");

	// super broker front and back end
	const std::string kSuperBroker_Frontend("5560");
	const std::string kSuperBroker_Backend("5561");

	// max cluster number
	const uint32_t kMaxCluster = 20;
	
	// client number in one cluster
	const uint32_t kClientNum = 1000;

	// worker number in one cluster
	const uint32_t kWorkerNum = 1000;

	// timeout while push cluster state in a cluster broker
	const uint32_t kTimeout_1000ms = 1000;

	// this id is used by worker to notify broker that it is alive at initial state
	const std::string globalConstID_ALIVE("WORKER_ALIVE");
}

// cluster configuration
struct ClusterCfg {
	uint32_t workerNum=constant::kWorkerNum;
	uint32_t clientNum=constant::kClientNum;
	std::string superBroker_IP = superBroker_IP;
	std::string statePullPort = constant::kPullPort;
	std::string superBrokerFrontend = constant::kSuperBroker_Frontend;
	std::string superBrokerBackend = constant::kSuperBroker_Backend;
};

class MessageHelper {
public:
	static bool stringToZMQMsg(zmq::message_t& msg, const std::string& payload);
	static bool ZMQMsgToString(const zmq::message_t& msg, std::string& str);
};

/**
 * @brief miscellaneous helper class
*/
class MiscHelper {
public:
	MiscHelper();
	virtual ~MiscHelper();

	static int randomInt();
	static float randomFloat();
private:
	static std::random_device randDevice_;
	static std::mt19937 generator_;
	static std::uniform_int_distribution<> distri_;
};

/**
 * @brief base class for none copiable calss
*/
class NonCopy {
public:
	NonCopy() {}
	virtual ~NonCopy() {}

	NonCopy(const NonCopy&) = delete;
	NonCopy& operator=(const NonCopy&) = delete;
};

/**
 * @brief async task, user should implement virtual void runTask() = 0
*/
class AsyncRun : public NonCopy {
public:
	AsyncRun() :stopTask_(false) {}
	virtual ~AsyncRun() { stopTask(); }

	void startTask() {
		taskHandle_ = std::thread(&AsyncRun::runTask, this);
	}
	void stopTask() {
		stopTask_ = true; 
		wait();
	}
	void wait() {
		if (taskHandle_.joinable()) taskHandle_.join();
	}
protected:
	virtual void runTask() = 0;
protected:
	std::thread taskHandle_;
	std::atomic_bool stopTask_;
};

/**
 * @brief push/pull for cluster broker state
*/
class ClusterStateProxy : public AsyncRun {
public:
	ClusterStateProxy(const std::string& pullPort = constant::kPullPort);
	virtual ~ClusterStateProxy();
public:
	void printClusterStateMap();
protected:
	virtual void runTask() override;
protected:
	[[deprecated("do not use this function since this class use push/pull instead of pub/sub")]]
	void subscribe(const std::string& topicPrefix="ClusterState");
private:
	zmq::context_t context_;
	zmq::socket_t socketPull_;
	std::map<std::string, ClusterStateInfoPtr> clusterInfoMap_;
}; 