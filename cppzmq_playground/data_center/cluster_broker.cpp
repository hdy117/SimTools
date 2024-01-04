#include "cluster_broker.h"

////////////////////////////////////////////////////////

Client::Client(const std::string& id, const std::string& port) {
	id_ = id;
	std::string frontEndAddr = "tcp://127.0.0.1:" + port;
	context_ = zmq::context_t(1);
	socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size() + 1);
	socket_.connect(frontEndAddr.c_str());
	LOG_0 << "client | id:" << id_ << ", connected to:" << frontEndAddr << "\n";

}
Client::~Client() {
	socket_.close();
	context_.close();
}
void Client::genTask(Task& task) {
	std::random_device rd;  // a seed source for the random number engine
	std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<int> distribInt(0, 1000);

	for (auto i = 0; i < task.size; ++i) {
		task.arr[i] = distribInt(gen) / 100.0;
	}

	task.meta.taskID = distribInt(gen);
	task.meta.taskType = 1;
}
void Client::runTask() {
	LOG_0 << id_ << " running.\n";
	while (!stopTask_) {
		sendRequest();
		std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_ClientReq));
	}
}
void Client::sendRequest() {
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
	auto rc = socket_.recv(taskResultMsg, zmq::recv_flags::none);

	// collect result
	TaskResult taskResult;
	memcpy(&taskResult, taskResultMsg.data(), taskResultMsg.size());
	LOG_0 << "client | id:" << id_ << ", sent taskID:" << task.meta.taskID <<
		", recv taskID:" << taskResult.meta.taskID << " taskResult:" << taskResult.sum;
}

////////////////////////////////////////////////////////

Worker::Worker(const std::string& id, const std::string& port) {
	workCounter_ = 0u;
	id_ = id;
	std::string backEndAddr = "tcp://127.0.0.1:" + port;
	context_ = zmq::context_t(1);
	socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	socket_.setsockopt(ZMQ_IDENTITY, id.c_str(), id.size() + 1);
	socket_.connect(backEndAddr.c_str());
	LOG_0 << "worker | id:" << id_ << ", connected to " << backEndAddr;

	// tell broker that I am alive
	zmq::message_t msgAlive(constant::globalConstID_ALIVE.size() + 1);
	memcpy(msgAlive.data(), constant::globalConstID_ALIVE.data(), constant::globalConstID_ALIVE.size() + 1);
	socket_.send(msgAlive, zmq::send_flags::none);
}
Worker::~Worker() {
	socket_.close();
	context_.close();
}
void Worker::runTask() {
	LOG_0 << id_ << " running.\n";
	while (!stopTask_) {
		LOG_0 << SEPERATOR << "\n";
		processImp();
		std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_ClientRep));
	}
}
void Worker::processImp() {
	// task
	Task task;
	TaskResult taskResult;

	// wait for task
	zmq::message_t clientIDMsg, taskMsg;
	auto rc = socket_.recv(clientIDMsg, zmq::recv_flags::none);
	rc = socket_.recv(taskMsg, zmq::recv_flags::none);

	std::string clientID(static_cast<const char*>(clientIDMsg.data()), clientIDMsg.size() - 1);
	memcpy(&task, taskMsg.data(), taskMsg.size());
	LOG_0 << "worker | id:" << id_ << ", got task: taskID:" << task.meta.taskID << " from " << clientID;

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
	LOG_0 << "worker | id:" << id_ << " processed task counter:" << workCounter_.load();
}

////////////////////////////////////////////////////////

LocalBalanceBroker::LocalBalanceBroker(const std::string& portFront, const std::string& portBack) {
	readyWorkerCount_ = 0;

	portFront_ = portFront;
	portBack_ = portBack;

	context_ = zmq::context_t(1);
	socketFrontEnd_ = zmq::socket_t(context_, zmq::socket_type::router);
	socketBackEnd_ = zmq::socket_t(context_, zmq::socket_type::router);

	// recv task high water mark
	socketFrontEnd_.setsockopt(ZMQ_RCVHWM, 1000);

	socketFrontEnd_.bind("tcp://0.0.0.0:" + portFront_);
	socketBackEnd_.bind("tcp://0.0.0.0:" + portBack_);

	LOG_0 << "local balancer forntend bind on:" << portFront_;
	LOG_0 << "local balancer backend bind on:" << portBack_;
}
LocalBalanceBroker::~LocalBalanceBroker() {
	socketBackEnd_.close();
	socketFrontEnd_.close();
	context_.close();
}
void LocalBalanceBroker::runTask() {
	LOG_0 << "local balancer running.\n";
	// poll item, poll backend first
	zmq::pollitem_t pollItems[] = { {socketBackEnd_, 0, ZMQ_POLLIN,0},{socketFrontEnd_, 0, ZMQ_POLLIN,0} };

	while (true) {
		// reset poll items
		for (auto i = 0; i < 2; ++i) {
			pollItems[i].fd = 0;
			pollItems[i].revents = 0;
		}

		// update ready worker count
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

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
			auto rc = socketBackEnd_.recv(msgWorkerID, zmq::recv_flags::none);
			rc = socketBackEnd_.recv(msgClientID, zmq::recv_flags::none);
			std::string workerID(static_cast<const char*>(msgWorkerID.data()), msgWorkerID.size() - 1);
			std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);

			// mark worker as ready
			workReadyQueue_.push(workerID);

			LOG_0 << "broker | got msg from worker:" << workerID << ", client ID:" << clientID;

			// a worker signal to broker that it is alive, no reply needed
			if (clientID == constant::globalConstID_ALIVE) {
				continue;
			}

			// get reply from worker
			zmq::message_t msgReply;
			rc = socketBackEnd_.recv(msgReply, zmq::recv_flags::none);

			// reply to client through frontend
			socketFrontEnd_.send(msgClientID, zmq::send_flags::sndmore);
			socketFrontEnd_.send(msgReply, zmq::send_flags::none);
		}
		if (pollItems[1].revents & ZMQ_POLLIN) {
			// if received a client task from frontend
			zmq::message_t msgClientID, msgTask;
			auto rc = socketFrontEnd_.recv(msgClientID, zmq::recv_flags::none);
			rc = socketFrontEnd_.recv(msgTask, zmq::recv_flags::none);

			std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);
			Task task;
			memcpy(&task, msgTask.data(), msgTask.size());

			LOG_0 << "broker | got msg from client:" << clientID << ", task id:" << task.meta.taskID;

			std::string workerID = workReadyQueue_.front();
			zmq::message_t msgWorkerID(workerID.size() + 1);
			memcpy(msgWorkerID.data(), workerID.c_str(), workerID.size() + 1);

			// forward to backend
			socketBackEnd_.send(msgWorkerID, zmq::send_flags::sndmore);
			socketBackEnd_.send(msgClientID, zmq::send_flags::sndmore);
			socketBackEnd_.send(msgTask, zmq::send_flags::none);

			workReadyQueue_.pop();
		}

		// update ready worker count
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());
	}
}
uint32_t LocalBalanceBroker::getReadyWorkerCount() {
	return readyWorkerCount_;
}

////////////////////////////////////////////////////////

OneCluster::OneCluster(const ClusterCfg& clusterCfg) {
	// clients and workers
	clients_.reserve(constant::kClientNum);
	workers_.reserve(constant::kWorkerNum);

	// cluster config
	clusterCfg_ = clusterCfg;
	clusterName_ = clusterCfg_.clusterName;

	// local load balancer
	localBalancer_ = std::make_shared<LocalBalanceBroker>(clusterCfg_.localFrontend, clusterCfg_.localBackend);

	// this cluster state reporter
	clusterState_ = std::make_shared<ClusterState>(clusterName_);
}
OneCluster::~OneCluster() {
	for (auto& client : clients_) {
		if (client.get() != nullptr)client->wait();
	}

	for (auto& worker : workers_) {
		if (worker.get() != nullptr)worker->wait();
	}
	localBalancer_->wait();
	clusterState_->wait();
}
void OneCluster::runTask() {
	// start local balancer task
	{
		localBalancer_->startTask();
	}

	// start cluster state task
	{
		clusterState_->startTask(); 
	}

	// start local clients and workers
	{
		for (auto& client : clients_) {
			if (client) client->startTask();
		}

		for (auto& worker : workers_) {
			if (worker) worker->startTask();
		}
	}

	// block waiting
	while (!stopTask_) {
		LOG_0 << clusterName_ << " has ready worker count:" << getReadyWorkerCount();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
uint32_t OneCluster::getReadyWorkerCount() {
	return localBalancer_->getReadyWorkerCount();
}

////////////////////////////////////////////////////////

void ClusterHelper::buildCluster(OneCluster& cluster) {
	auto& clients = cluster.getClients();
	auto& workers = cluster.getWorkers();
	const auto& clusterCfg = cluster.getClusterCfg();

	for (auto i = 0; i < clusterCfg.clientNum; ++i) {
		std::string clientID = cluster.getClusterName() + std::string("::") + std::string("client_") + std::to_string(i);
		clients.push_back(std::make_shared<Client>(clientID, clusterCfg.localFrontend));
	}

	for (auto i = 0; i < clusterCfg.workerNum; ++i) {
		std::string workerID = cluster.getClusterName() + std::string("::") + std::string("worker_") + std::to_string(i);
		workers.push_back(std::make_shared<Worker>(workerID, clusterCfg.localBackend));
	}
}