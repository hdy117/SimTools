#pragma once

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <queue>
#include <random>
#include <iomanip>

#include "sim_log.h"

#include "zmq.hpp"

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
	const static uint32_t kMaxNameSize = 256;
	char clusterName[kMaxNameSize] = {'\0'};
	uint32_t readyWorkerCount = 0;
};

namespace constant {
	const std::string kXPubPort("5556");
	const std::string kXSubPort("5557");
	const uint32_t kMaxCluster = 20;
}

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

class AsyncRun;
using AsyncRunPtr = std::shared_ptr<AsyncRun>;

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
 * @brief xpub and xsub proxy for cluster broker state
*/
class ClusterStateProxy : public AsyncRun {
public:
	ClusterStateProxy(const std::string& xpubPort = constant::kXPubPort, const std::string& xsubPort = constant::kXSubPort);
	virtual ~ClusterStateProxy();
protected:
	virtual void runTask() override;
private:
	zmq::context_t context_;
	zmq::socket_t socketXPub_, socketXSub_;
};