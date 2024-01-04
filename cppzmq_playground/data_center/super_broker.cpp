#include "super_broker.h"

SuperBroker::SuperBroker(const SuperBrokerCfg& superBrokerCfg) {
	superBrokerCfg_ = superBrokerCfg;

	clusterStateBroker_ = std::make_shared<ClusterStateBroker>(superBrokerCfg_.statePullPort);
}
SuperBroker::~SuperBroker() {
}
void SuperBroker::runTask() {
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
	const int64_t kPollTimeout = 2;
	while (!stopTask_) {
		// poll on subscribe socket
		zmq::pollitem_t pollItems[] = { {socketPull_, 0, ZMQ_POLLIN,0} };
		zmq::poll(pollItems, 1, kPollTimeout);

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
void ClusterStateBroker::subscribe(const std::string& topicPrefix) {
	// set subscribe and unsubscribe topics
	for (auto i = 1; i <= constant::kMaxCluster; ++i) {
		std::string topic = topicPrefix + std::to_string(i);
		socketPull_.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
	}
}