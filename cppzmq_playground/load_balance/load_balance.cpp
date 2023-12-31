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
		context_ = zmq::context_t(1);
		socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
		socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size() + 1);
		socket_.connect(frontEndAddr.c_str());
		SPDLOG_INFO("client | id:{}, connected to:{}", id_, frontEndAddr);
	}
	virtual ~Client() {
		if(handle_.joinable()) handle_.join();
		socket_.close();
		context_.close();
	}
public:
	void startThread() {
		handle_ = std::thread(&Client::sendRequest, this);
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

		// wait for result
		zmq::message_t taskResultMsg;
		socket_.recv(taskResultMsg, zmq::recv_flags::none);

		// collect result
		TaskResult taskResult;
		memcpy(&taskResult, taskResultMsg.data(), taskResultMsg.size());
		SPDLOG_INFO("client | id:{}, sent taskID:{}, recv taskID:{}, , taskResult:{:.4f}", id_, task.meta.taskID, taskResult.meta.taskID, taskResult.sum);
	}
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::thread handle_;
	std::string id_;
	std::atomic_bool stop_;
};

class Worker {
public:
	explicit Worker(const std::string& id, const std::string& port = "5557") {
		stop_ = false;
		workCounter_ = 0u;
		id_ = id;
		std::string backEndAddr = "tcp://127.0.0.1:" + port;
		context_ = zmq::context_t(1);
		socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
		socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size() + 1);
		socket_.connect(backEndAddr.c_str());
		SPDLOG_INFO("worker | id:{}, connected to {}", id_, backEndAddr);

		// tell broker that I am alive
		zmq::message_t msgAlive(globalConstID_ALIVE.size() + 1);
		memcpy(msgAlive.data(), globalConstID_ALIVE.data(), globalConstID_ALIVE.size() + 1);
		socket_.send(msgAlive, zmq::send_flags::none);
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
			std::this_thread::sleep_for(std::chrono::milliseconds(111));
		}
	}
	void processImp() {
		// task
		Task task;
		TaskResult taskResult;

		// wait for task
		zmq::message_t clientIDMsg, taskMsg;
		socket_.recv(clientIDMsg, zmq::recv_flags::none);
		socket_.recv(taskMsg, zmq::recv_flags::none);

		std::string clientID(static_cast<const char*>(clientIDMsg.data()), clientIDMsg.size() - 1);
		memcpy(&task, taskMsg.data(), taskMsg.size());
		SPDLOG_INFO("worker | id:{}, got task: taskID:{} from {}", id_, task.meta.taskID, clientID);

		// do work
		taskResult.meta = task.meta;
		taskResult.sum = 0;
		for (auto i = 0; i < task.size; ++i) {
			taskResult.sum += task.arr[i];
		}

		// send result
		zmq::message_t taskResultMsg(sizeof(TaskResult));
		memcpy(taskResultMsg.data(), &taskResult, sizeof(TaskResult));
		socket_.send(clientIDMsg, zmq::send_flags::sndmore);
		socket_.send(taskResultMsg, zmq::send_flags::none);

		workCounter_++;
		SPDLOG_INFO("worker | id:{} counter:{}", id_, workCounter_.load());
	}
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::thread handle_;
	std::string id_;
	std::atomic_bool stop_;
	std::atomic<uint32_t> workCounter_;
};

class BalanceBroker {
public:
	BalanceBroker(const std::string& portFront = "5556", const std::string& portBack = "5557") {
		portFront_ = portFront;
		portBack_ = portBack;

		context_ = zmq::context_t(1);
		socketFrontEnd_ = zmq::socket_t(context_, zmq::socket_type::router);
		socketBackEnd_ = zmq::socket_t(context_, zmq::socket_type::router);

		socketFrontEnd_.bind("tcp://0.0.0.0:" + portFront_);
		socketBackEnd_.bind("tcp://0.0.0.0:" + portBack_);
	}
	virtual ~BalanceBroker(){
		socketBackEnd_.close();
		socketFrontEnd_.close();
		context_.close();
	}
public:
	void serve() {
		// poll item, poll backend first
		zmq::pollitem_t pollItems[] = { {socketBackEnd_, 0, ZMQ_POLLIN,0},{socketFrontEnd_, 0, ZMQ_POLLIN,0} };

		while (true) {
			// reset poll items
			for (auto i = 0; i < 2; ++i) {
				pollItems[i].fd = 0;
				pollItems[i].revents = 0;
			}

			// wait worker to be ready
			if (workReadyQueue_.empty()) {
				// if no ready worker, broker wait until there is at least one ready worker, then do balancing things
				zmq::poll(&pollItems[0], 1, -1);
			}
			else {
				// if there be ready worker, do balancing things
				zmq::poll(&pollItems[0], 2, -1);
			}

			if (pollItems[0].revents & ZMQ_POLLIN) {
				// got something from worker, may be a signal of ready(globalConstID_ALIVE) or reply of a task
				zmq::message_t msgWorkerID, msgClientID;
				socketBackEnd_.recv(msgWorkerID, zmq::recv_flags::none);
				socketBackEnd_.recv(msgClientID, zmq::recv_flags::none);
				std::string workerID(static_cast<const char*>(msgWorkerID.data()), msgWorkerID.size() - 1);
				std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);
				
				// mark worker as ready
				workReadyQueue_.push(workerID);

				SPDLOG_INFO("broker | got msg from worker:{}, client ID:{}", workerID, clientID);

				// a worker signal to broker that it is alive, no reply needed
				if (clientID == globalConstID_ALIVE) {
					continue;
				}

				// get reply from worker
				zmq::message_t msgReply;
				socketBackEnd_.recv(msgReply, zmq::recv_flags::none);

				// reply to client through frontend
				socketFrontEnd_.send(msgClientID, zmq::send_flags::sndmore);
				socketFrontEnd_.send(msgReply, zmq::send_flags::none);
			}
			if (pollItems[1].revents & ZMQ_POLLIN) {
				// if received a client task from frontend
				zmq::message_t msgClientID, msgTask;
				socketFrontEnd_.recv(msgClientID, zmq::recv_flags::none);
				socketFrontEnd_.recv(msgTask, zmq::recv_flags::none);

				std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);
				Task task;
				memcpy(&task, msgTask.data(), msgTask.size());
				
				SPDLOG_INFO("broker | got msg from client:{}, task id:{}", clientID, task.meta.taskID);
				
				std::string workerID = workReadyQueue_.front();
				zmq::message_t msgWorkerID(workerID.size() + 1);
				memcpy(msgWorkerID.data(), workerID.c_str(), workerID.size() + 1);

				// forward to backend
				socketBackEnd_.send(msgWorkerID, zmq::send_flags::sndmore);
				socketBackEnd_.send(msgClientID, zmq::send_flags::sndmore);
				socketBackEnd_.send(msgTask, zmq::send_flags::none);

				workReadyQueue_.pop();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
private:
	zmq::context_t context_;
	zmq::socket_t socketFrontEnd_, socketBackEnd_;
	std::string portFront_, portBack_;
	std::queue<std::string> workReadyQueue_;
};

int main() {
	initSpdlog();

	const int clientNum = 5, workerNum = 3;
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

	BalanceBroker broker;

	broker.serve();

	return 0;
}