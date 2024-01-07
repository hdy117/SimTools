#include "super_broker.h"

SuperBroker::SuperBroker(const SuperBrokerCfg& superBrokerCfg) {
	superBrokerCfg_ = superBrokerCfg;
	context_ = zmq::context_t(1);

	// state puller
	std::string subAddr = "tcp://0.0.0.0:" + superBrokerCfg_.pullState_Port;
	socketPull_ = zmq::socket_t(context_, zmq::socket_type::pull);
	socketPull_.bind(subAddr);
	LOG_0 << "super broker bind on state puller:" << subAddr << "\n";

	// task port
	socketCloudTask_ = zmq::socket_t(context_, zmq::socket_type::router);
	std::string taskAddr = "tcp://0.0.0.0:" + superBrokerCfg_.task_Port;
	socketCloudTask_.bind(taskAddr);
	LOG_0 << "super broker bind on task backend:" << taskAddr << "\n";
}
SuperBroker::~SuperBroker() {
}
void SuperBroker::runTask() {
	while (!stopTask_) {
		// poll
		zmq::pollitem_t pollItems[] = { 
			{socketPull_, 0, ZMQ_POLLIN,0}, 
			{socketCloudTask_,0,ZMQ_POLLIN,0} 
		};
		zmq::poll(pollItems, 2, constant::kTimeout_10ms);

		// if new cluster state arrived
		if (pollItems[0].revents & ZMQ_POLLIN) {
			statePuller();
		}
		if (pollItems[1].revents & ZMQ_POLLIN) {
			taskRouter();
		}
	}
}
void SuperBroker::statePuller() {
	// sub one cluster info message
	zmq::message_t clusterStateMsg;
	auto rc = socketPull_.recv(clusterStateMsg, zmq::recv_flags::none);

	// copy data from subscribed message
	ClusterStateInfoPtr clusterStatePtr = std::make_shared<ClusterStateInfo>();
	memcpy(clusterStatePtr.get(), clusterStateMsg.data(), clusterStateMsg.size());

	// add to clusterInfoMap_
	clusterInfoMap_[std::string(clusterStatePtr->clusterName)] = clusterStatePtr;

	//LOG_EVERY_N(INFO, 1000) << "super-broker got: " << clusterStatePtr->clusterName
	//	<< ", ready worker count:" << clusterStatePtr->readyWorkerCount << "\n";
}
void SuperBroker::taskRouter() {
	// recv task from cluster
	zmq::message_t clusterIDMsg, metaMsg, taskPayloadMsg;
	auto rc = socketCloudTask_.recv(clusterIDMsg, zmq::recv_flags::none);
	rc = socketCloudTask_.recv(metaMsg, zmq::recv_flags::none);
	rc = socketCloudTask_.recv(taskPayloadMsg, zmq::recv_flags::none);

	// get task meta
	TaskMeta taskMeta;
	memcpy(&taskMeta, metaMsg.data(), metaMsg.size());

	if (taskMeta.taskType == TaskType::ALIVE_SIGNAL) {
		LOG_0 << "got alive signal from " << taskMeta.taskAddr.fromClusterID << "\n";
		return;
	}

	// target cluster to route this task
	std::string targetCluster("");

	if (taskMeta.taskDirection == TaskDirection::TASK_SUBMIT) {
		LOG_0 << "super broker got a request, fromClusterID:" << taskMeta.taskAddr.fromClusterID
			<< ", toClusterID:" << taskMeta.taskAddr.toClusterID
			<<", fromID:"<<taskMeta.taskAddr.fromID
			<<", toID:"<<taskMeta.taskAddr.toID
			<< "\n";
		if (std::string(taskMeta.taskAddr.fromID).empty() || std::string(taskMeta.taskAddr.fromClusterID).empty()) {
			LOG_ERROR << "got null task.\n";
			return;
		}
		// route this request to proper cluster
		uint32_t maxCount = 0;

		// find proper cluster to route
		for (auto& pair : clusterInfoMap_) {
			targetCluster = pair.second->readyWorkerCount > maxCount ? pair.first : targetCluster;
			maxCount = std::max(maxCount, pair.second->readyWorkerCount);
		}

		if (maxCount == 0 || targetCluster.empty()) {
			// route request back to client with error, send this request back with error info
			MessageHelper::swapBuffer(taskMeta.taskAddr.fromID, taskMeta.taskAddr.toID);
			MessageHelper::swapBuffer(taskMeta.taskAddr.fromClusterID, taskMeta.taskAddr.toClusterID);

			LOG_ERROR << "can not find proper cluster to route request, reply with rejected from cluster:" << taskMeta.taskAddr.fromClusterID
				<< ", from client:" << taskMeta.taskAddr.fromID
				<< ", to clusterID:" << taskMeta.taskAddr.toClusterID
				<< ", toID:" << taskMeta.taskAddr.toID
				<< ", request rejected.\n";

			TaskReply reply;
			reply.sum = 0.0;
			taskMeta.taskDirection = TaskDirection::TASK_REPLY_REJECTED;

			// send task back
			zmq::message_t clusterMsgID, replyMsg(sizeof(TaskReply));
			MessageHelper::stringToZMQMsg(clusterMsgID, std::string(taskMeta.taskAddr.toClusterID));

			memcpy(metaMsg.data(), &taskMeta, sizeof(TaskMeta));
			memcpy(replyMsg.data(), &reply, sizeof(TaskReply));

			socketCloudTask_.send(clusterMsgID, zmq::send_flags::sndmore);
			socketCloudTask_.send(metaMsg, zmq::send_flags::sndmore);
			socketCloudTask_.send(replyMsg, zmq::send_flags::none);
			return;
		}
	}
	else {
		LOG_0 << "super broker got a reply, fromClusterID:" << taskMeta.taskAddr.fromClusterID
			<< ", toClusterID:" << taskMeta.taskAddr.toClusterID
			<< ", fromID:" << taskMeta.taskAddr.fromID
			<< ", toID:" << taskMeta.taskAddr.toID
			<< "\n";
		// route this reply to proper cluster
		targetCluster = taskMeta.taskAddr.toClusterID;
	}

	// update address
	MessageHelper::copyStringToBuffer(taskMeta.taskAddr.toClusterID, targetCluster);

	LOG_0 << "super broker | route message from cluster:" << taskMeta.taskAddr.fromClusterID 
		<< ", to cluster:" << taskMeta.taskAddr.toClusterID <<"\n";

	// route
	zmq::message_t toClusterIDMsg;
	MessageHelper::stringToZMQMsg(toClusterIDMsg, targetCluster);

	memcpy(metaMsg.data(), &taskMeta, sizeof(TaskMeta));

	socketCloudTask_.send(toClusterIDMsg, zmq::send_flags::sndmore);
	socketCloudTask_.send(metaMsg, zmq::send_flags::sndmore);
	socketCloudTask_.send(taskPayloadMsg, zmq::send_flags::none);
}

