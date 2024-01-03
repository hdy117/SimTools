#include "cluster_state.h"

ClusterState::ClusterState(const std::string& clusterName, 
	const std::string& pullerIP, const std::string& pullPort) {
	LOG_0 << SEPERATOR << "\n";

	clusterName_ = clusterName;
	pullerIP_ = pullerIP;

	memcpy(clusterStateInfo_.clusterName, clusterName_.c_str(), clusterName_.size() + 1);
	clusterStateInfo_.readyWorkerCount = 0;

	context_ = zmq::context_t(1);

	pullPort_ = pullPort;
	std::string subAddr = "tcp://" + pullerIP_ + ":" + pullPort_;
	socketPush_ = zmq::socket_t(context_, zmq::socket_type::push);
	socketPush_.connect(subAddr);
	LOG_0 << clusterName << " push connect to :" << subAddr << ".\n";
}
ClusterState::~ClusterState() {
	socketPush_.close();
	LOG_0 << "cluster " << clusterName_ << " quit.\n";
}
void ClusterState::runTask() {
	LOG_0 << "hi, this is cluster state" << clusterName_ << " thread.\n";

	while (!stopTask_) {
		LOG_0 << SEPERATOR << "\n";

		// send cluster state infomation msg 
		zmq::message_t stateInfoMsg(sizeof(ClusterStateInfo));
		clusterStateInfo_.readyWorkerCount = MiscHelper::randomInt();
		memcpy(stateInfoMsg.data(), &clusterStateInfo_, sizeof(ClusterStateInfo));
		socketPush_.send(stateInfoMsg, zmq::send_flags::none);
		LOG_0 << clusterStateInfo_.clusterName << " ready worker count:" << clusterStateInfo_.readyWorkerCount << "\n";

		std::this_thread::sleep_for(std::chrono::milliseconds(constant::kTimeout_1000ms));
	}
}