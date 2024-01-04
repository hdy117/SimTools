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
		process();
		std::this_thread::sleep_for(std::chrono::milliseconds(MiscHelper::randomInt()));
	}
}
void Worker::process() {
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

LocalBalanceBroker::LocalBalanceBroker(zmq::context_t* context,
	const std::string& portFront, const std::string& portBack) {
	readyWorkerCount_ = 0;

	portFront_ = portFront;
	portBack_ = portBack;

	context_ = context;
	socketFrontEnd_ = zmq::socket_t(*context_, zmq::socket_type::router);
	socketBackEnd_ = zmq::socket_t(*context_, zmq::socket_type::router);

	// recv task high water mark
	//socketFrontEnd_.setsockopt(ZMQ_RCVHWM, 1000);

	socketFrontEnd_.bind("tcp://0.0.0.0:" + portFront_);
	socketBackEnd_.bind("tcp://0.0.0.0:" + portBack_);

	LOG_0 << "local balancer forntend bind on:" << portFront_;
	LOG_0 << "local balancer backend bind on:" << portBack_;

	// socket for cloud
	socketLocalFe_ = zmq::socket_t(*context_, zmq::socket_type::pair);
	socketLocalFe_.connect("inproc://cloud_task");
}
LocalBalanceBroker::~LocalBalanceBroker() {
	socketBackEnd_.close();
	socketFrontEnd_.close();
}
void LocalBalanceBroker::runTask() {
	LOG_0 << "local balancer running.\n";

	// poll item, poll backend, frontend, cloudend
	zmq::pollitem_t pollItems[] = { {socketBackEnd_, 0, ZMQ_POLLIN,0},{socketFrontEnd_, 0, ZMQ_POLLIN,0},{socketLocalFe_, 0, ZMQ_POLLIN,0} };

	while (true) {
		// update ready worker count
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

		// poll
		zmq::poll(pollItems, 3, constant::kTimeout_1ms);

		// if local reply arrived
		if (pollItems[0].revents & ZMQ_POLLIN) {
			// got something from worker, may be a signal of ready(globalConstID_ALIVE) or reply of a task
			zmq::message_t msgWorkerID, msgClientID;
			auto rc = socketBackEnd_.recv(msgWorkerID, zmq::recv_flags::none);
			rc = socketBackEnd_.recv(msgClientID, zmq::recv_flags::none);
			std::string workerID(static_cast<const char*>(msgWorkerID.data()), msgWorkerID.size() - 1);
			std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);

			// mark worker as ready
			workReadyQueue_.push(workerID);
			readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

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
		
		// if local task arrived, forward to local worker or super broker(another cluster)
		if (pollItems[1].revents & ZMQ_POLLIN) {
			// if received a client task from frontend
			zmq::message_t msgClientID, msgTask;
			auto rc = socketFrontEnd_.recv(msgClientID, zmq::recv_flags::none);
			rc = socketFrontEnd_.recv(msgTask, zmq::recv_flags::none);

			std::string clientID(static_cast<const char*>(msgClientID.data()), msgClientID.size() - 1);
			Task task;
			memcpy(&task, msgTask.data(), msgTask.size());

			LOG_0 << "broker | got msg from client:" << clientID << ", task id:" << task.meta.taskID;

			// forward task to local worker or super broker
			if (workReadyQueue_.size() > 0) {
				// forward to local workers if there is at least one ready worker
				std::string workerID = workReadyQueue_.front();
				zmq::message_t msgWorkerID(workerID.size() + 1);
				memcpy(msgWorkerID.data(), workerID.c_str(), workerID.size() + 1);

				// forward to backend
				socketBackEnd_.send(msgWorkerID, zmq::send_flags::sndmore);
				socketBackEnd_.send(msgClientID, zmq::send_flags::sndmore);
				socketBackEnd_.send(msgTask, zmq::send_flags::none);

				// update ready queue
				workReadyQueue_.pop();
				readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());
			}
			else {
				// forward to super broker
				socketLocalFe_.send(msgClientID, zmq::send_flags::sndmore);
				socketLocalFe_.send(msgTask, zmq::send_flags::none);
			}
		}

		// if super broker reply something, forward to client
		if (pollItems[2].revents & ZMQ_POLLIN) {
			zmq::message_t clientMsg, taskReplyMsg;
			auto rc = socketLocalFe_.recv(clientMsg, zmq::recv_flags::none);
			rc = socketLocalFe_.recv(taskReplyMsg, zmq::recv_flags::none);

			socketFrontEnd_.send(clientMsg, zmq::send_flags::sndmore);
			socketFrontEnd_.send(taskReplyMsg, zmq::send_flags::none);
		}
	}
}
uint32_t LocalBalanceBroker::getReadyWorkerCount() {
	return readyWorkerCount_;
}

////////////////////////////////////////////////////////

OneCluster::OneCluster(const ClusterCfg& clusterCfg) {
	// create a 2 threads context
	context_ = zmq::context_t(2);

	// clients and workers
	clients_.reserve(constant::kClientNum);
	workers_.reserve(constant::kWorkerNum);

	// cluster config
	clusterCfg_ = clusterCfg;
	clusterName_ = clusterCfg_.clusterName;
	memcpy(stateInfo_.clusterName, clusterName_.c_str(), clusterName_.size());

	// local load balancer
	localBalancer_ = std::make_shared<LocalBalanceBroker>(&context_, clusterCfg_.localFrontend, clusterCfg_.localBackend);

	// this cluster state reporter
	clusterState_ = std::make_shared<ClusterStateReporter>(clusterName_);

	// socket for cloud task distribution, socketCloudFe_ is inproc with local load balance
	socketCloudFe_ = zmq::socket_t(context_, zmq::socket_type::pair);
	socketCloudFe_.bind("inproc://cloud_task");

	// socket for cloud task distribution, socketCloudBe_ is dealer with super broker task port
	socketCloudBe_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	std::string superBroker_taskAddr = "tcp://" + clusterCfg_.superBrokerCfg.ip + ":" + clusterCfg_.superBrokerCfg.task_Port;
	socketCloudBe_.setsockopt(ZMQ_IDENTITY, clusterName_.c_str(), clusterName_.size());
	socketCloudBe_.connect(superBroker_taskAddr);
	LOG_0 << clusterName_ << " connect to super broker task port:" << superBroker_taskAddr << "\n";
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
		// get cluster state info and set it to ClusterStateReporter class
		stateInfo_.readyWorkerCount = getReadyWorkerCount();
		LOG_0 << clusterName_ << " has ready worker count:" << stateInfo_.readyWorkerCount;
		clusterState_->setClusterState(stateInfo_);

		// task or reply that should be forward to cloud and to client
		cloudTaskRouting();

		// sleep for 10 ms
		//std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_10ms));
	}
}
void OneCluster::cloudTaskRouting() {
	// poll on task reply and new task
	zmq::pollitem_t pollItems[] = { {socketCloudBe_, 0, ZMQ_POLLIN,0}, {socketCloudFe_, 0, ZMQ_POLLIN,0} };
	zmq::poll(pollItems, 2, std::chrono::milliseconds(constant::kTimeout_10ms));

	// forward reply to client
	if (pollItems[1].revents & ZMQ_POLLIN) {
		// get message from socketCloudBe_
		zmq::message_t clientIDMsg, taskReplyMsg;
		auto rc = socketCloudBe_.recv(clientIDMsg, zmq::recv_flags::none);
		rc = socketCloudBe_.recv(taskReplyMsg, zmq::recv_flags::none);

		// forward task to client
		socketCloudFe_.send(clientIDMsg, zmq::send_flags::sndmore);
		socketCloudFe_.send(taskReplyMsg, zmq::send_flags::none);
	}

	// forward task to super broker
	if (pollItems[0].revents & ZMQ_POLLIN) {
		// get message from socketCloudFe_
		zmq::message_t clientIDMsg, taskMsg;
		auto rc = socketCloudFe_.recv(clientIDMsg, zmq::recv_flags::none);
		rc = socketCloudFe_.recv(taskMsg, zmq::recv_flags::none);

		// forward task to super broker
		socketCloudBe_.send(clientIDMsg, zmq::send_flags::sndmore);
		socketCloudBe_.send(taskMsg, zmq::send_flags::none);
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