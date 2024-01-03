#include "cluster_state.h"

ClusterState::ClusterState(const std::string& clusterName, const std::string& xpubxsubIP, const std::string& xpubPort, const std::string& xsubPort) {
	LOG_0 << SEPERATOR << "\n";

	clusterName_ = clusterName;
	xpubxsubIP_ = xpubxsubIP;

	memcpy(clusterStateInfo_.clusterName, clusterName_.c_str(), clusterName_.size() + 1);
	clusterStateInfo_.readyWorkerCount = 0;

	context_ = zmq::context_t(1);

	xsubPort_ = xsubPort;
	std::string xsubAddr = "tcp://" + xpubxsubIP_ + ":" + xsubPort_;
	socketStatePub_ = zmq::socket_t(context_, zmq::socket_type::pub);
	socketStatePub_.connect(xsubAddr);
	LOG_0 << clusterName << " pub connect to :" << xsubAddr << ".\n";

	xpubPort_ = xpubPort;
	std::string xpubAddr = "tcp://" + xpubxsubIP_ + ":" + xpubPort_;
	socketStateSub_ = zmq::socket_t(context_, zmq::socket_type::sub);
	socketStateSub_.connect(xpubAddr);

	// set subscribe and unsubscribe topics
	setSubscribe();
	
	LOG_0 << clusterName << " sub connect to :" << xpubAddr << ".\n";
}
ClusterState::~ClusterState() {
	socketStatePub_.close();
	socketStateSub_.close();
	LOG_0 << "cluster " << clusterName_ << " quit.\n";
}
void ClusterState::setSubscribe() {
	// set subscribe and unsubscribe topics
	for (auto i = 1; i <= constant::kMaxCluster; ++i) {
		std::string topic = std::string("ClusterState") + std::to_string(i);
		socketStateSub_.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
	}
	socketStateSub_.setsockopt(ZMQ_UNSUBSCRIBE, clusterName_.c_str(), clusterName_.size());
}
void ClusterState::runTask() {
	LOG_0 << "hi, this is cluster state" << clusterName_ << " thread.\n";
	const int kPollTimeout = 1000;
	const int kPollTimeoutEach = kPollTimeout / constant::kMaxCluster;
	std::chrono::high_resolution_clock::time_point t1, t2;

	// poll
	zmq::pollitem_t pollItems[] = { {socketStateSub_, 0, ZMQ_POLLIN,0} };

	while (!stopTask_) {
		LOG_0 << SEPERATOR << "\n";

		// recv information
		ClusterStateInfo oneClusterStateInfo;
		zmq::message_t oneClusterStateInfoMsg;

		// poll on sub socket with timeout kPollTimeout ms
		zmq::poll(pollItems, 1, kPollTimeout);

		// if data polled
		if (pollItems[0].revents & ZMQ_POLLIN) {
			socketStateSub_.recv(oneClusterStateInfoMsg, zmq::recv_flags::none);
			memcpy(&oneClusterStateInfo, oneClusterStateInfoMsg.data(), oneClusterStateInfoMsg.size());
			LOG_0 << clusterName_ << " got:" << oneClusterStateInfo.clusterName
				<< ", ready worker count:" << oneClusterStateInfo.readyWorkerCount << "\n";
		}
		else {
			// send msg if no data 
			zmq::message_t stateInfoMsg(sizeof(ClusterStateInfo));
			clusterStateInfo_.readyWorkerCount = MiscHelper::randomInt();
			memcpy(stateInfoMsg.data(), &clusterStateInfo_, sizeof(ClusterStateInfo));
			socketStatePub_.send(stateInfoMsg, zmq::send_flags::none);
			LOG_0 << clusterStateInfo_.clusterName << " ready worker count:" << clusterStateInfo_.readyWorkerCount << "\n";
		}
	}
}
/* N publisher nodes publish N messages and 1 subscriber subscribes 1 message each time.
 * This is not good. should poll N times in one subscribe node to avoid messages consume all buffer of recv node.
*/
//void ClusterState::runTask() {
//	LOG_0 << "hi, this is cluster state" << clusterName_ << " thread.\n";
//	const int kPollTimeout = 1000;
//	const int kPollTimeoutEach = kPollTimeout / constant::kMaxCluster;
//	std::chrono::high_resolution_clock::time_point t1, t2;
//
//	// poll
//	zmq::pollitem_t pollItems[] = { {socketStateSub_, 0, ZMQ_POLLIN,0} };
//
//	while (!stopTask_) {
//		LOG_0 << SEPERATOR << "\n";
//
//		// clear poll infomation
//		//pollItems[0].fd = 0;
//		//pollItems[0].revents = 0;
//
//		// timestamp
//		t1 = std::chrono::high_resolution_clock::now();
//		
//		// recv information
//		ClusterStateInfo oneClusterStateInfo;
//		zmq::message_t oneClusterStateInfoMsg;
//
//		// poll on sub socket with timeout kPollTimeout ms
//		for (auto i = 0; i < constant::kMaxCluster; ++i) {
//			zmq::poll(pollItems, 1, kPollTimeoutEach);
//
//			// if data polled
//			if (pollItems[0].revents & ZMQ_POLLIN) {
//				socketStateSub_.recv(oneClusterStateInfoMsg, zmq::recv_flags::none);
//				memcpy(&oneClusterStateInfo, oneClusterStateInfoMsg.data(), oneClusterStateInfoMsg.size());
//				LOG_0 << clusterName_ << " got:" << oneClusterStateInfo.clusterName
//					<< ", ready worker count:" << oneClusterStateInfo.readyWorkerCount << "\n";
//			}
//		}
//		// timestamp
//		t2 = std::chrono::high_resolution_clock::now();
//
//		// make sure sleep for one second
//		auto elapsedTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
//		LOG_0 << clusterName_ << " elapsedTime_ms:" << elapsedTime_ms << ", sleep for:" << kPollTimeout - elapsedTime_ms << "\n";
//		if (elapsedTime_ms < kPollTimeout) {
//			std::this_thread::sleep_for(std::chrono::milliseconds(kPollTimeout - elapsedTime_ms));
//		}
//
//		// send msg
//		zmq::message_t stateInfoMsg(sizeof(ClusterStateInfo));
//		clusterStateInfo_.readyWorkerCount = MiscHelper::randomInt();
//		memcpy(stateInfoMsg.data(), &clusterStateInfo_, sizeof(ClusterStateInfo));
//		socketStatePub_.send(stateInfoMsg, zmq::send_flags::none);
//		LOG_0 << clusterStateInfo_.clusterName << " ready worker count:" << clusterStateInfo_.readyWorkerCount << "\n";
//	}
//}