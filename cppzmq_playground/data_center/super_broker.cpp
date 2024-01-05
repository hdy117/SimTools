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

	LOG_0 << "super-broker got: " << clusterStatePtr->clusterName
		<< ", ready worker count:" << clusterStatePtr->readyWorkerCount << "\n";
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

	// target cluster to route this task
	std::string targetCluster;

	if (taskMeta.taskDirection = TaskDirection::TASK_SUBMIT) {
		// route this request to proper cluster
		uint32_t maxCount = 0;

		// find proper cluster to route
		for (auto& pair : clusterInfoMap_) {
			targetCluster = pair.second->readyWorkerCount > maxCount ? pair.first : targetCluster;
			maxCount = std::max(maxCount, pair.second->readyWorkerCount);
		}

		if (targetCluster.empty()) {
			LOG_ERROR << "can not find proper cluster to route request, from cluster:" << taskMeta.taskAddr.fromClusterID 
				<< ", from client:" << taskMeta.taskAddr.fromID << ", request rejected.\n";
			return;
		}
	}
	else {
		// route this reply to proper cluster
		targetCluster = taskMeta.taskAddr.toClusterID;
	}

	// route
	zmq::message_t toClusterIDMsg;
	MessageHelper::stringToZMQMsg(toClusterIDMsg, targetCluster);

	socketCloudTask_.send(toClusterIDMsg, zmq::send_flags::sndmore);
	socketCloudTask_.send(metaMsg, zmq::send_flags::sndmore);
	socketCloudTask_.send(taskPayloadMsg, zmq::send_flags::none);
}

