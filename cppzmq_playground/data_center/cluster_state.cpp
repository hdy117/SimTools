#include "cluster_state.h"

ClusterState::ClusterState(const std::string& clusterName, const std::string& xpubxsubIP, const std::string& xpubPort, const std::string& xsubPort) {
	clusterName_ = clusterName;
	xpubxsubIP_ = xpubxsubIP;
	xpubPort_ = xpubPort;
	xsubPort_ = xsubPort;
	context_ = zmq::context_t(1);
	socketStatePub_ = zmq::socket_t(context_, zmq::socket_type::pub);
	socketStateSub_ = zmq::socket_t(context_, zmq::socket_type::sub);

	std::string xpubAddr = "tcp://" + xpubxsubIP_ + ":" + xpubPort_;
	std::string xsubAddr = "tcp://" + xpubxsubIP_ + ":" + xsubPort_;

	socketStatePub_.connect(xsubAddr);
	socketStateSub_.connect(xpubAddr);

	LOG_0 << clusterName << " pub connect to :" << xsubAddr << ".\n";
	LOG_0 << clusterName << " sub connect to :" << xpubAddr << ".\n";
}
ClusterState::~ClusterState() {
	socketStatePub_.close();
	socketStateSub_.close();
	LOG_0 << "cluster " << clusterName_ << " quit.\n";
}
void ClusterState::runTask() {
	LOG_0 << "hi, this is cluster state" << clusterName_ << " thread.\n";

	/*zmq::pollitem_t pollItems[] = { {socketStateSub_, 0, ZMQ_POLLIN,0} };*/

	while (!stopTask_) {
		LOG_0 << SEPERATOR << "\n";

		// clear poll
		/*pollItems[0].fd = 0;
		pollItems[0].revents = 0;*/
		zmq::pollitem_t pollItems[] = { {socketStateSub_, 0, ZMQ_POLLIN,0} };
		
		// poll on sub socket with timeout 1000ms
		zmq::poll(pollItems, 1, 10);
		LOG_0 << clusterName_<<" pollItems[0].revents:" << pollItems[0].revents << "\n";
		
		// if data polled
		if (pollItems[0].revents & ZMQ_POLLIN) {
			ClusterStateInfo oneClusterStateInfo;
			zmq::message_t oneClusterStateInfoMsg;
			socketStateSub_.recv(oneClusterStateInfoMsg, zmq::recv_flags::none);

			memcpy(&oneClusterStateInfo, oneClusterStateInfoMsg.data(), oneClusterStateInfoMsg.size());
			LOG_0 << clusterName_ << " got:" << oneClusterStateInfo.clusterName << ", ready worker count:" << oneClusterStateInfo.readyWorkerCount << "\n";
		}
		else {
			
		}

		// publish this cluster state info
		ClusterStateInfo thisClusterStateInfo;
		memcpy(thisClusterStateInfo.clusterName, clusterName_.c_str(), clusterName_.size() + 1);
		thisClusterStateInfo.readyWorkerCount = MiscHelper::randomInt();
		zmq::message_t stateInfoMsg(sizeof(ClusterStateInfo));
		memcpy(stateInfoMsg.data(), &thisClusterStateInfo, sizeof(ClusterStateInfo));
		socketStatePub_.send(stateInfoMsg, zmq::send_flags::none);
		LOG_0 << thisClusterStateInfo.clusterName << " ready worker count:" << thisClusterStateInfo.readyWorkerCount << "\n";

		// sleep for one second
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}