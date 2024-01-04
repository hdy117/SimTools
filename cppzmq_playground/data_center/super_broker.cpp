#include "super_broker.h"

SuperBroker::SuperBroker(const SuperBrokerCfg& superBrokerCfg) {
	superBrokerCfg_ = superBrokerCfg;

	// state puller
	clusterStateBroker_ = std::make_shared<ClusterStateBroker>(superBrokerCfg_.pullState_Port);

	// task port
	context_ = zmq::context_t(1);
	socketBackend_ = zmq::socket_t(context_, zmq::socket_type::router);
	std::string taskAddr = "tcp://0.0.0.0:" + superBrokerCfg_.task_Port;
	socketBackend_.bind(taskAddr);
	LOG_0 << "super broker bind on task backend:" << taskAddr << "\n";
}
SuperBroker::~SuperBroker() {
}
void SuperBroker::runTask() {
	// start cluster state puller
	clusterStateBroker_->startTask();

	while (!stopTask_) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		// poll
		zmq::pollitem_t pollItems[] = { {socketBackend_,0,ZMQ_POLLIN,0} };
		zmq::poll(pollItems, 1, constant::kTimeout_10ms);

		// if new task arrived
		if (pollItems[0].revents & ZMQ_POLLIN) {
			// recv from cluster
			zmq::message_t clusterIDMsg, clientIDMsg, taskMsg;
			auto rc = socketBackend_.recv(clusterIDMsg, zmq::recv_flags::none);
			rc = socketBackend_.recv(clientIDMsg, zmq::recv_flags::none);
			rc = socketBackend_.recv(taskMsg, zmq::recv_flags::none);

			// 
		}
	}
}

////////////////////////////////

ClusterStateBroker::ClusterStateBroker(const std::string& pullPort) {
	context_ = zmq::context_t(1);

	// bind to pull port
	std::string subAddr = "tcp://0.0.0.0:" + pullPort;
	socketPull_ = zmq::socket_t(context_, zmq::socket_type::pull);
	socketPull_.bind(subAddr);
	LOG_0 << "cluster state proxy bind on " << subAddr << "\n";
}
ClusterStateBroker::~ClusterStateBroker() {
	socketPull_.close();
	context_.close();
}
void ClusterStateBroker::runTask() {
	LOG_0 << "hi, this is cluster state proxy thread.\n";

	while (!stopTask_) {
		// poll on subscribe socket
		zmq::pollitem_t pollItems[] = { {socketPull_, 0, ZMQ_POLLIN,0} };
		zmq::poll(pollItems, 1, constant::kTimeout_1000ms);

		if (pollItems[0].revents & ZMQ_POLLIN) {
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
	}
}
void ClusterStateBroker::printClusterStateMap() {
	// print cluster state array
	for (const auto& pairClusterState : clusterInfoMap_) {
		LOG_0 << "super-broker got: " << pairClusterState.second->clusterName
			<< ", ready worker count:" << pairClusterState.second->readyWorkerCount << "\n";
	}
}
