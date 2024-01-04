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

enum TaskDirection {
	TASK_SUBMIT = 0, TASK_REPLY, TASK_REPLY_WITH_ERROR, TASK_REPLY_REJECTED
};

struct TaskMeta {
	uint64_t taskID;
	uint64_t taskType;
	TaskDirection taskDirection;
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

// information of cluster
struct ClusterStateInfo {
	const static uint32_t kMaxNameSize = 128;
	char clusterName[kMaxNameSize] = {'\0'};
	uint32_t readyWorkerCount = 0;
};

class AsyncRun;
class ClusterStateBroker;
using AsyncRunPtr = std::shared_ptr<AsyncRun>;
using ClusterStateInfoPtr = std::shared_ptr<ClusterStateInfo>;
using ClusterStateBrokerPtr = std::shared_ptr<ClusterStateBroker>;

namespace constant {
	// super broker IP
	const std::string kSuperBroker_IP("127.0.0.1");

	// pull port to collect all cluster broker state in super broker
	const std::string kSuperBroker_PullStatePort("55550");
	// super broker front and back end
	const std::string kSuperBroker_TaskPort("55560");

	// local port of front and back end for client and worker in one cluster
	const std::string kLocal_Frontend[] = { "55580", "55581", "55582" };
	const std::string kLocal_backend[] = { "55590","55591","55592" };

	// max cluster number
	const uint32_t kMaxCluster = 20;
	
	// client number in one cluster
	const uint32_t kClientNum = 3;

	// worker number in one cluster
	const uint32_t kWorkerNum = 3;

	// timeout while push cluster state in a cluster broker
	const uint32_t kTimeout_1000ms = 1000;

	// timeout 10 ms
	const uint32_t kTimeout_10ms = 10;

	// timeout 1 ms
	const uint32_t kTimeout_1ms = 1;

	// timeout value for client
	const uint32_t kTimeout_ClientReq = 10;

	// this id is used by worker to notify broker that it is alive at initial state
	const std::string globalConstID_ALIVE("WORKER_ALIVE");
}

// super broker config
struct SuperBrokerCfg {
	std::string ip = constant::kSuperBroker_IP;
	std::string pullState_Port = constant::kSuperBroker_PullStatePort;
	std::string task_Port = constant::kSuperBroker_TaskPort;
};

// cluster configuration
struct ClusterCfg {
	// name of cluster
	std::string clusterName = std::string("Cluster");

	// number of workers and clients in one cluster
	uint32_t workerNum=constant::kWorkerNum;
	uint32_t clientNum=constant::kClientNum;

	// super broker ip and port info
	SuperBrokerCfg superBrokerCfg;

	// local load balance broker info
	std::string localFrontend = constant::kLocal_Frontend[0];
	std::string localBackend = constant::kLocal_backend[0];
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