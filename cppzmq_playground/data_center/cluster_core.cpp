#include "cluster_core.h"

/////////////////////////////////////////////

bool MessageHelper::stringToZMQMsg(zmq::message_t& msg,
	const std::string& payload) {
	// payload.size() + 1 is important
	zmq::message_t tmpMsg(payload.size() + 1);
	memcpy(tmpMsg.data(), payload.c_str(), payload.size() + 1);
	msg = std::move(tmpMsg);
	return true;
}

bool MessageHelper::ZMQMsgToString(const zmq::message_t& msg,
	std::string& str) {
	// msg.size() - 1 is important
	str.clear();
	str = std::string(static_cast<const char*>(msg.data()), msg.size() - 1);
	return true;
}

/////////////////////////////////////////////
std::random_device MiscHelper::randDevice_;
std::mt19937 MiscHelper::generator_(MiscHelper::randDevice_());
std::uniform_int_distribution<> MiscHelper::distri_ = std::uniform_int_distribution<>(0, 10000);

MiscHelper::MiscHelper() {
}
MiscHelper::~MiscHelper() {}

int MiscHelper::randomInt() {
	return distri_(generator_);
}
float MiscHelper::randomFloat() {
	return randomInt() / 10000.0f;
}

////////////////////////////////

ClusterStateProxy::ClusterStateProxy(const std::string& pullPort) {
	context_ = zmq::context_t(1);

	// bind to pull port
	std::string subAddr = "tcp://0.0.0.0:" + pullPort;
	socketPull_ = zmq::socket_t(context_, zmq::socket_type::pull);
	socketPull_.bind(subAddr);
	LOG_0 << "cluster state proxy bind on " << subAddr << "\n";
}
ClusterStateProxy::~ClusterStateProxy() {
	socketPull_.close();
	context_.close();
}
void ClusterStateProxy::runTask() {
	LOG_0 << "hi, this is cluster state proxy thread.\n";
	const int64_t kPollTimeout = 2;
	while (!stopTask_) {
		// poll on subscribe socket
		zmq::pollitem_t pollItems[] = { {socketPull_, 0, ZMQ_POLLIN,0} };
		zmq::poll(pollItems, 1, kPollTimeout);
		
		if (pollItems[0].revents & ZMQ_POLLIN) {
			// sub one cluster info message
			zmq::message_t clusterStateMsg;
			socketPull_.recv(clusterStateMsg, zmq::recv_flags::none);
			
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
void ClusterStateProxy::printClusterStateMap() {
	// print cluster state array
	for (const auto& pairClusterState : clusterInfoMap_) {
		LOG_0 <<"super-broker got: "<< pairClusterState.second->clusterName 
			<< ", ready worker count:" << pairClusterState.second->readyWorkerCount << "\n";
	}
}
void ClusterStateProxy::subscribe(const std::string& topicPrefix) {
	// set subscribe and unsubscribe topics
	for (auto i = 1; i <= constant::kMaxCluster; ++i) {
		std::string topic = topicPrefix + std::to_string(i);
		socketPull_.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
	}
}