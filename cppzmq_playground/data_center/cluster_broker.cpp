#include "cluster_broker.h"

////////////////////////////////////////////////////////

Client::Client(const std::string& id, const std::string& port) {
	id_ = id;
	std::string frontEndAddr = "tcp://127.0.0.1:" + port;
	context_ = zmq::context_t(1);
	socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	socket_.setsockopt(ZMQ_IDENTITY, id_.c_str(), id_.size() + 1);
	socket_.connect(frontEndAddr.c_str());
	LOG_0 << "client | id:" << id_ << ", connected to:" << frontEndAddr << "\n";

}
Client::~Client() {
	socket_.close();
	context_.close();
}
void Client::genTask(TaskMeta& meta, TaskRequest& task) {
	std::random_device rd;  // a seed source for the random number engine
	std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<int> distribInt(0, 1000);

	for (auto i = 0; i < task.size; ++i) {
		task.arr[i] = distribInt(gen) / 1000.0;
	}

	meta.taskID = distribInt(gen);
}
void Client::runTask() {
	LOG_0 << id_ << " running.\n";
	while (!stopTask_) {
		sendRequest();
		//std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_ClientReq));
	}
}
void Client::sendRequest() {
	// generate task
	TaskMeta taskMeta;
	TaskRequest taskReq;
	genTask(taskMeta, taskReq);

	// task meta info
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.fromID, id_);
	taskMeta.taskDirection = TaskDirection::TASK_SUBMIT;
	taskMeta.taskType = TaskType::NormalTask;

	// prepare task meta and request message
	zmq::message_t metaMsg(sizeof(TaskMeta)), taskMsg(sizeof(TaskRequest));
	memcpy(metaMsg.data(), &taskMeta, sizeof(TaskMeta));
	memcpy(taskMsg.data(), &taskReq, sizeof(TaskRequest));

	// send task meta and request
	socket_.send(metaMsg, zmq::send_flags::sndmore);
	socket_.send(taskMsg, zmq::send_flags::none);

	LOG_1 << "client | id:" << id_ << ", sent fromID:" << taskMeta.taskAddr.fromID << ", taskID:" << taskMeta.taskID << "\n";

	// wait for result
	zmq::message_t taskResultMsg;
	auto rc = socket_.recv(metaMsg, zmq::recv_flags::none);
	rc = socket_.recv(taskResultMsg, zmq::recv_flags::none);

	// collect recv meta and reply
	TaskMeta taskMetaRecv;
	TaskReply taskReply;
	memcpy(&taskMetaRecv, metaMsg.data(), metaMsg.size());
	if (taskMetaRecv.taskDirection == TaskDirection::TASK_REPLY) {
		memcpy(&taskReply, taskResultMsg.data(), taskResultMsg.size());
		LOG_0 << "client | id:" << id_ << ", sent taskID:" << taskMeta.taskID <<
			", recv taskID:" << taskMetaRecv.taskID
			<< ", fromClusterID:" << taskMetaRecv.taskAddr.fromClusterID
			<< ", fromID:" << taskMetaRecv.taskAddr.fromID
			<< ", taskReply:" << taskReply.sum << ".\n";
	}
	else {
		LOG_0 << "client | id:" << id_ << ", sent taskID:" << taskMeta.taskID <<
			", recv taskID:" << taskMetaRecv.taskID
			<< ", fromClusterID:" << taskMetaRecv.taskAddr.fromClusterID
			<< ", fromID:" << taskMetaRecv.taskAddr.fromID
			<< ", rejected.\n";

		// sleep for 1000ms if this request is rejected.
		std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_1000ms));
	}
}

////////////////////////////////////////////////////////

Worker::Worker(const std::string& id, const std::string& port) {
	workCounter_ = 0u;
	id_ = id;
	std::string backEndAddr = "tcp://127.0.0.1:" + port;
	context_ = zmq::context_t(1);
	socket_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	socket_.setsockopt(ZMQ_IDENTITY, id_.c_str(), id_.size() + 1);
	socket_.connect(backEndAddr);
	LOG_0 << "worker | id:" << id_ << ", connected to " << backEndAddr;

	// tell broker that I am alive
	TaskMeta meta;
	MessageHelper::copyStringToBuffer(meta.taskAddr.fromID, id_);
	meta.taskDirection = TaskDirection::TASK_REPLY;
	meta.taskType = TaskType::ALIVE_SIGNAL;

	// signal alive
	zmq::message_t metaMsg(sizeof(TaskMeta)), replyMsg(1);
	memcpy(metaMsg.data(), &meta, sizeof(TaskMeta));
	memcpy(replyMsg.data(), "", 1);
	socket_.send(metaMsg, zmq::send_flags::sndmore);
	socket_.send(replyMsg, zmq::send_flags::none);
	LOG_0 << "worker | id:" << id_ << ", alive signal sent.";
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
		//std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}
void Worker::process() {
	// task
	TaskMeta taskMeta;
	TaskRequest taskReq;
	TaskReply taskResult;

	// wait for task
	zmq::message_t metaMsg, taskMsg;
	auto rc = socket_.recv(metaMsg, zmq::recv_flags::none);
	rc = socket_.recv(taskMsg, zmq::recv_flags::none);

	memcpy(&taskMeta, metaMsg.data(), metaMsg.size());
	memcpy(&taskReq, taskMsg.data(), taskMsg.size());

	LOG_0 << "worker | id:" << id_ << ", got task: taskID:" << taskMeta.taskID
		<< ", fromClusterID:" << taskMeta.taskAddr.fromClusterID
		<< ", fromID:" << taskMeta.taskAddr.fromID
		<< "\n";

	// update address, route id
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toID, std::string(taskMeta.taskAddr.fromID));
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.fromID, id_);
	taskMeta.taskDirection = TaskDirection::TASK_REPLY;

	// do work
	taskResult.sum = 0.0;
	for (auto i = 0; i < taskReq.size; ++i) {
		taskResult.sum += taskReq.arr[i];
	}

	// send result
	zmq::message_t metaMsgReply(sizeof(TaskMeta)), taskResultMsg(sizeof(TaskReply));
	memcpy(metaMsgReply.data(), &taskMeta, sizeof(TaskMeta));
	memcpy(taskResultMsg.data(), &taskResult, sizeof(TaskReply));

	socket_.send(metaMsgReply, zmq::send_flags::sndmore);
	socket_.send(taskResultMsg, zmq::send_flags::none);

	workCounter_++;
	LOG_1 << "worker | id:" << id_ << " processed task counter:" << workCounter_.load();
}

////////////////////////////////////////////////////////

LocalBalanceBroker::LocalBalanceBroker(const ClusterCfg& clusterCfg) {
	clusterCfg_ = clusterCfg;
	clusterName_ = clusterCfg_.clusterName;
	readyWorkerCount_ = 0;

	context_ = zmq::context_t(2);
	socketFrontEnd_ = zmq::socket_t(context_, zmq::socket_type::router);
	socketBackEnd_ = zmq::socket_t(context_, zmq::socket_type::router);

	// recv task high water mark
	//socketFrontEnd_.setsockopt(ZMQ_RCVHWM, 1000);

	socketFrontEnd_.bind("tcp://0.0.0.0:" + clusterCfg_.localFrontend);
	socketBackEnd_.bind("tcp://0.0.0.0:" + clusterCfg_.localBackend);

	LOG_0 << "local balancer forntend bind on:" << clusterCfg_.localFrontend;
	LOG_0 << "local balancer backend bind on:" << clusterCfg_.localBackend;

	// socket for cloud task routing, socket address is clusterName_
	socketCloudTask_ = zmq::socket_t(context_, zmq::socket_type::dealer);
	socketCloudTask_.setsockopt(ZMQ_IDENTITY, clusterName_.c_str(), clusterName_.size() + 1);
	std::string cloudTaskAddr = "tcp://" + clusterCfg_.superBrokerCfg.ip + ":" + clusterCfg_.superBrokerCfg.task_Port;
	socketCloudTask_.connect(cloudTaskAddr);
	LOG_0 << clusterName_ << " connect to super broker on " << cloudTaskAddr << "\n";

	// signal alive to super broker
	TaskMeta meta;
	MessageHelper::copyStringToBuffer(meta.taskAddr.fromClusterID, clusterName_);
	meta.taskType = TaskType::ALIVE_SIGNAL;

	zmq::message_t metaMsg(sizeof(TaskMeta)), replyMsg(1);

	memcpy(metaMsg.data(), &meta, sizeof(TaskMeta));
	memcpy(replyMsg.data(), "", 1);

	socketCloudTask_.send(metaMsg, zmq::send_flags::sndmore);
	socketCloudTask_.send(replyMsg, zmq::send_flags::none);

	LOG_0 << "cluster router | id:" << clusterName_ << ", alive signal sent.";
}
LocalBalanceBroker::~LocalBalanceBroker() {
	socketBackEnd_.close();
	socketFrontEnd_.close();
	socketCloudTask_.close();
	context_.close();
}
void LocalBalanceBroker::runTask() {
	LOG_0 << "local balancer running.\n";

	// poll item, poll backend, frontend, cloudend
	zmq::pollitem_t pollItems[] = {
		{socketBackEnd_, 0, ZMQ_POLLIN,0},
		{socketFrontEnd_, 0, ZMQ_POLLIN,0},
		{socketCloudTask_, 0, ZMQ_POLLIN,0}
	};

	while (true) {
		// update ready worker count
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

		// poll
		zmq::poll(pollItems, 3, constant::kTimeout_1ms);

		// got a reply from worker
		if (pollItems[0].revents & ZMQ_POLLIN) {
			routeLocalReply();
		}

		// if local task arrived, forward to local worker or super broker(another cluster)
		if (pollItems[1].revents & ZMQ_POLLIN) {
			routeLocalRequest();
		}

		// if super broker reply something, forward to client
		if (pollItems[2].revents & ZMQ_POLLIN) {
			routeCloud();
		}
	}
}
void LocalBalanceBroker::routeCloud() {
	// recv from super broker
	zmq::message_t msgMeta, taskPayloadMsg;
	auto rc = socketCloudTask_.recv(msgMeta, zmq::recv_flags::none);
	rc = socketCloudTask_.recv(taskPayloadMsg, zmq::recv_flags::none);

	// copy meta message
	TaskMeta taskMeta;
	memcpy(&taskMeta, msgMeta.data(), msgMeta.size());

	if (taskMeta.taskDirection == TaskDirection::TASK_SUBMIT) {
		// got a request task from cloud and route to local worker
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

		// if we have at least one ready worker
		if (readyWorkerCount_ > 0)
		{
			std::string workerID = workReadyQueue_.front();
			workReadyQueue_.pop();
			readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

			zmq::message_t workerIDMsg;
			MessageHelper::stringToZMQMsg(workerIDMsg, workerID);

			// update address, request from cloud to local worker in this cluster
			MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toClusterID, clusterName_);
			MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toID, workerID);

			// route to local worker
			socketBackEnd_.send(workerIDMsg, zmq::send_flags::sndmore);
			socketBackEnd_.send(msgMeta, zmq::send_flags::sndmore);
			socketBackEnd_.send(taskPayloadMsg, zmq::send_flags::none);

			LOG_0 << "route request from cluster:" << taskMeta.taskAddr.fromClusterID
				<< ", to cluster:" << taskMeta.taskAddr.toClusterID
				<< ", workerID:" << workerID
				<< "\n";
		}
		else {
			// route request back to client with error, send this request back with error info
			MessageHelper::swapBuffer(taskMeta.taskAddr.fromID, taskMeta.taskAddr.toID);
			MessageHelper::swapBuffer(taskMeta.taskAddr.fromClusterID, taskMeta.taskAddr.toClusterID);

			LOG_ERROR << "broker | routeCloud no ready worker(rejected) --> this clusterName_:" << clusterName_
				<< ", fromClusterID:" << taskMeta.taskAddr.fromClusterID
				<< ", toClusterID:" << taskMeta.taskAddr.toClusterID
				<< ", fromID:" << taskMeta.taskAddr.fromID
				<< ", toID:" << taskMeta.taskAddr.toID
				<< "\n";

			taskMeta.taskDirection = TaskDirection::TASK_REPLY_REJECTED;
			memcpy(msgMeta.data(), &taskMeta, sizeof(TaskMeta));

			// send task back
			zmq::message_t clusterMsgID, replyMsg(sizeof(TaskReply));
			MessageHelper::stringToZMQMsg(clusterMsgID, std::string(taskMeta.taskAddr.toClusterID));

			TaskReply reply{ 0.0 };
			memcpy(replyMsg.data(), &reply, sizeof(TaskReply));

			socketCloudTask_.send(msgMeta, zmq::send_flags::sndmore);
			socketCloudTask_.send(replyMsg, zmq::send_flags::none);
		}
	}
	else {
		// got a reply task and route to client
		std::string clientID(taskMeta.taskAddr.toID);

		zmq::message_t clientIDMsg;
		MessageHelper::stringToZMQMsg(clientIDMsg, taskMeta.taskAddr.toID);

		socketFrontEnd_.send(clientIDMsg, zmq::send_flags::sndmore);
		socketFrontEnd_.send(msgMeta, zmq::send_flags::sndmore);
		socketFrontEnd_.send(taskPayloadMsg, zmq::send_flags::none);

		LOG_0 << "route reply from cluster:" << taskMeta.taskAddr.fromClusterID << ", to cluster:" << taskMeta.taskAddr.toClusterID << "\n";
	}
}
void LocalBalanceBroker::routeLocalRequest() {
	// if received a client task from frontend
	zmq::message_t msgClientID, msgMeta, msgTask;
	auto rc = socketFrontEnd_.recv(msgClientID, zmq::recv_flags::none);
	rc = socketFrontEnd_.recv(msgMeta, zmq::recv_flags::none);
	rc = socketFrontEnd_.recv(msgTask, zmq::recv_flags::none);

	// get task meta info
	TaskMeta taskMeta;
	memcpy(&taskMeta, msgMeta.data(), msgMeta.size());

	// update address, add from cluster
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.fromClusterID, clusterName_);

	// forward task to local worker or super broker
	if (workReadyQueue_.size() > 0) {
		// forward to local workers if there is at least one ready worker
		std::string workerID = workReadyQueue_.front();

		// update ready queue
		workReadyQueue_.pop();
		readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

		zmq::message_t msgWorkerID;
		MessageHelper::stringToZMQMsg(msgWorkerID, workerID);

		LOG_1 << "broker | got msg from client:" << taskMeta.taskAddr.fromID <<
			", task id:" << taskMeta.taskID << ", send to worker:" << workerID << "\n";

		// update address, add target address information
		MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toClusterID, clusterName_);
		MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toID, workerID);
		memcpy(msgMeta.data(), &taskMeta, sizeof(TaskMeta));

		// forward to backend
		socketBackEnd_.send(msgWorkerID, zmq::send_flags::sndmore);
		socketBackEnd_.send(msgMeta, zmq::send_flags::sndmore);
		socketBackEnd_.send(msgTask, zmq::send_flags::none);
	}
	else {
		// forward to super broker
		memcpy(msgMeta.data(), &taskMeta, sizeof(TaskMeta));
		socketCloudTask_.send(msgMeta, zmq::send_flags::sndmore);
		socketCloudTask_.send(msgTask, zmq::send_flags::none);

		LOG_0 << "broker | got msg from client:" << taskMeta.taskAddr.fromID <<
			", task id:" << taskMeta.taskID << ", send to super broker, taskDirection:" << taskMeta.taskDirection << ".\n";
	}
}
void LocalBalanceBroker::routeLocalReply() {
	// got something from worker, may be a signal of alive or reply of a task
	zmq::message_t msgWorkerID, msgMeta, msgReply;
	auto rc = socketBackEnd_.recv(msgWorkerID, zmq::recv_flags::none);
	rc = socketBackEnd_.recv(msgMeta, zmq::recv_flags::none);
	rc = socketBackEnd_.recv(msgReply, zmq::recv_flags::none);

	// get task meta info
	TaskMeta taskMeta;
	memcpy(&taskMeta, msgMeta.data(), msgMeta.size());

	// mark worker as ready
	workReadyQueue_.push(std::string(taskMeta.taskAddr.fromID));
	readyWorkerCount_ = static_cast<uint32_t>(workReadyQueue_.size());

	// alive signal received from local worker
	if (taskMeta.taskType == TaskType::ALIVE_SIGNAL) {
		LOG_0 << "broker | got alive signal from worker:" << taskMeta.taskAddr.fromID;
		return;
	}

	// update address, router cluster
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toClusterID, std::string(taskMeta.taskAddr.fromClusterID));
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.fromClusterID, clusterName_);
	memcpy(msgMeta.data(), &taskMeta, sizeof(TaskMeta));

	// route to local client or another cluster
	if (clusterName_ == std::string(taskMeta.taskAddr.toClusterID)) {
		// route to local client
		zmq::message_t clientIDMsg;
		MessageHelper::stringToZMQMsg(clientIDMsg, taskMeta.taskAddr.toID);

		// send to local client
		socketFrontEnd_.send(clientIDMsg, zmq::send_flags::sndmore);
		socketFrontEnd_.send(msgMeta, zmq::send_flags::sndmore);
		socketFrontEnd_.send(msgReply, zmq::send_flags::none);
	}
	else {
		LOG_0 << "broker | got msg from worker, fromID:" << taskMeta.taskAddr.fromID << ", toID:" << taskMeta.taskAddr.toID
			<< ", fronClusterID:" << taskMeta.taskAddr.fromClusterID << ", toClusterID:" << taskMeta.taskAddr.toClusterID
			<< ", taskMeta.taskDirection:" << taskMeta.taskDirection
			<< "\n";

		// route to another cluster
		auto sd = socketCloudTask_.send(msgMeta, zmq::send_flags::sndmore);
		sd = socketCloudTask_.send(msgReply, zmq::send_flags::none);
		LOG_0 << "send state:" << sd.value() << "\n";
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
	MessageHelper::copyStringToBuffer(stateInfo_.clusterName, clusterName_);

	// local load balancer
	localBalancer_ = std::make_shared<LocalBalanceBroker>(clusterCfg_);

	// this cluster state reporter
	clusterState_ = std::make_shared<ClusterStateReporter>(clusterName_);

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
		//LOG_0 << clusterName_ << " has ready worker count:" << stateInfo_.readyWorkerCount;
		clusterState_->setClusterState(stateInfo_);
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
		std::string clientID = std::string("client_") + std::to_string(i);
		clients.push_back(std::make_shared<Client>(clientID, clusterCfg.localFrontend));
	}

	for (auto i = 0; i < clusterCfg.workerNum; ++i) {
		std::string workerID = std::string("worker_") + std::to_string(i);
		workers.push_back(std::make_shared<Worker>(workerID, clusterCfg.localBackend));
	}

}