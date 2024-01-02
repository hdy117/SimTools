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

#include "message/location.pb.h"
#include "message/basic.pb.h"
#include "message/trajectory.pb.h"


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

// this id is used by worker to notify broker that it is alive at initial state
const std::string globalConstID_ALIVE("WORKER_ALIVE");

class Client;
using ClientPtr = std::shared_ptr<Client>;

class Worker;
using WorkerPtr = std::shared_ptr<Worker>;

class Client {
public:
	explicit Client(const std::string& id, const std::string& port = "5556") {
		stop_ = false;
		id_ = id;
		std::string frontEndAddr = "tcp://127.0.0.1:" + port;
		context_ = zmq::context_t(2);
		socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
		socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size() + 1);
		socket_.connect(frontEndAddr.c_str());
		LOG_0 << "client | id:" << id_ << ", connected to:" << frontEndAddr << "\n";
	}
	virtual ~Client() {
		if (handle_.joinable()) handle_.join();
		if (handlePoll_.joinable()) handlePoll_.join();
		socket_.close();
		context_.close();
	}
public:
	/**
	 * @brief start thread send and poll
	*/
	void startThread() {
		handle_ = std::thread(&Client::sendRequest, this);
		handlePoll_ = std::thread(&Client::poll, this);
	}
	/**
	 * @brief stop client
	*/
	void stopClient() {
		stop_ = true;
	}
	const std::string& getID() { return id_; }
	void genTask(Task& task) {
		std::random_device rd;  // a seed source for the random number engine
		std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<int> distribInt(0, 1000);

		for (auto i = 0; i < task.size; ++i) {
			task.arr[i] = distribInt(gen) / 100.0;
		}

		task.meta.taskID = distribInt(gen);
		task.meta.taskType = 1;
	}
private:
	void poll() {
		zmq::pollitem_t pollItems[] = { {socket_, 0 ,ZMQ_POLLIN, 0} };
		TaskResult taskResult;
		while (!stop_) {
			pollItems[0].fd = 0;
			pollItems[0].revents = 0;

			// poll with timeout 10ms
			auto polledItems = zmq::poll(pollItems, 1, kTimeOut);
			if (polledItems == 0) {
				LOG_1 << "client | got nothing from server within " << kTimeOut << "[ms].\n";
				//SPDLOG_INFO("client | got nothing from server within {}[ms]", kTimeOut);
			}
			else if(pollItems[0].revents & ZMQ_POLLIN) {
				zmq::message_t msgReply;
				socket_.recv(msgReply, zmq::recv_flags::none);
				memcpy(&taskResult, msgReply.data(), msgReply.size());
				LOG_0 << "client | got task:" << taskResult.meta.taskID << ", value:" << taskResult.sum << ".\n";
			}
		}
	}
	void sendRequest() {
		for (auto i = 0; i < 3; ++i) {
			sendRequestImp();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	void sendRequestImp() {
		// generate task
		Task task;
		genTask(task);

		// prepare task message
		zmq::message_t taskMsg(sizeof(Task));
		memcpy(taskMsg.data(), &task, sizeof(Task));

		// send task
		socket_.send(taskMsg, zmq::send_flags::none);
		LOG_0 << "client | id:" << id_ << ", sent taskID:" << task.meta.taskID << "\n";
	}
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::thread handle_, handlePoll_;
	std::string id_;
	std::atomic_bool stop_;
	const long kTimeOut = 100;
};

class Worker {
public:
	explicit Worker(const std::string& id, const std::string& port = "5557") {
		stop_ = false;
		id_ = id;
		std::string backEndAddr = "tcp://127.0.0.1:" + port;
		context_ = zmq::context_t(1);
		socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
		socket_.connect(backEndAddr.c_str());
		LOG_0 << "worker | id:" << id_ << ", connected to " << backEndAddr << "\n";

	}
	virtual ~Worker() {
		if (handle_.joinable()) handle_.join();
		socket_.close();
		context_.close();
	}
public:
	void startThread() {
		handle_ = std::thread(&Worker::process, this);
	}
	const std::string& getID() { return id_; }
private:
	void process() {
		while (!stop_) {
			processImp();
		}
	}
	void processImp() {
		// task
		Task task;
		TaskResult taskResult;

		// wait for task
		zmq::message_t brokerIDMsg, clientIDMsg, taskMsg;
		//socket_.recv(brokerIDMsg, zmq::recv_flags::none);
		socket_.recv(clientIDMsg, zmq::recv_flags::none);
		socket_.recv(taskMsg, zmq::recv_flags::none);

		//std::string brokerID(static_cast<const char*>(brokerIDMsg.data()), brokerIDMsg.size() - 1);
		std::string clientID(static_cast<const char*>(clientIDMsg.data()), clientIDMsg.size() - 1);
		memcpy(&task, taskMsg.data(), taskMsg.size());
		//LOG_0 << "worker | id:" << id_ << ", got task: taskID:" << task.meta.taskID << " from client:" << clientID << ", broker:" << brokerID << "\n";
		LOG_0 << "worker | id:" << id_ << ", got task: taskID:" << task.meta.taskID << " from client:" << clientID << "\n";

		// do work
		taskResult.meta = task.meta;
		taskResult.sum = 0;
		for (auto i = 0; i < task.size; ++i) {
			taskResult.sum += task.arr[i];
		}

		// send result multiple times
		for (auto i = 0; i < 10; ++i) {
			zmq::message_t taskResultMsg(sizeof(TaskResult)), copyIDMsg, copyReplyMsg;
			memcpy(taskResultMsg.data(), &taskResult, sizeof(TaskResult));
			copyIDMsg.copy(clientIDMsg);
			copyReplyMsg.copy(taskMsg);
			//socket_.send(brokerIDMsg, zmq::send_flags::sndmore);
			socket_.send(copyIDMsg, zmq::send_flags::sndmore);
			socket_.send(copyReplyMsg, zmq::send_flags::none);
			//std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::thread handle_;
	std::string id_;
	std::atomic_bool stop_;
};

class BrokerBase {
public:
	virtual void serve() = 0;
};

class ProxyBroker : public BrokerBase {
public:
	ProxyBroker(const std::string& portFront = "5556", const std::string& portBack = "5557") {
		portFront_ = portFront;
		portBack_ = portBack;

		context_ = zmq::context_t(1);
		socketFrontend_ = zmq::socket_t(context_, zmq::socket_type::router);
		socketBackend_ = zmq::socket_t(context_, zmq::socket_type::dealer);
		socketBackend_.setsockopt(ZMQ_IDENTITY, "BrokerDealer", 13);

		socketFrontend_.bind("tcp://0.0.0.0:" + portFront_);
		socketBackend_.bind("tcp://0.0.0.0:" + portBack_);
	}
	virtual ~ProxyBroker() {
		socketBackend_.close();
		socketFrontend_.close();
		context_.close();
	}
public:
	virtual void serve() override {
		zmq::proxy(socketFrontend_, socketBackend_, nullptr);
	}
private:
	zmq::context_t context_;
	zmq::socket_t socketFrontend_, socketBackend_;
	std::string portFront_, portBack_;
	std::queue<std::string> workReadyQueue_;
};

int main(int argc, char* argv[]) {
	//google::InitGoogleLogging(argv[0]);
	initSpdlog();

	FLAGS_v = 1;

	const int clientNum = 1, workerNum = 1;
	std::vector<ClientPtr> clients;
	std::vector<WorkerPtr> workers;

	// create clients
	for (auto i = 0; i < clientNum; ++i) {
		clients.emplace_back(new Client(std::string("stub_" + std::to_string(i))));
		clients.at(i)->startThread();
	}

	// create workers
	for (auto i = 0; i < workerNum; ++i) {
		workers.emplace_back(new Worker(std::string("worker_" + std::to_string(i))));
		workers.at(i)->startThread();
	}

	BrokerBase* broker = new ProxyBroker();

	broker->serve();

	delete broker;

	return 0;
}