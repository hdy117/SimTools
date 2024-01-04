#include "cluster_state.h"

ClusterStateReporter::ClusterStateReporter(const std::string& clusterName,
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

	socketFe_ = zmq::socket_t(context_, zmq::socket_type::pair);
	socketBe_ = zmq::socket_t(context_, zmq::socket_type::pair);

	// it's ok, each peer can initiate sending action.
	// in this case, socketFe_ initiate sending action
	socketBe_.connect("inproc://state_updated");
	socketFe_.bind("inproc://state_updated");
}
ClusterStateReporter::~ClusterStateReporter() {
	socketPush_.close();
	LOG_0 << "cluster " << clusterName_ << " quit.\n";
}
void ClusterStateReporter::runTask() {
	LOG_0 << "hi, this is cluster state:" << clusterName_ << " thread.\n";

	while (!stopTask_) {
		LOG_0 << SEPERATOR << "\n";

		// poll
		zmq::pollitem_t pollItems[] = { {socketBe_,0,ZMQ_POLLIN,0} };
		zmq::poll(pollItems, 1, constant::kTimeout_1000ms);

		// if new cluster state info arrived
		if (pollItems[0].revents & ZMQ_POLLIN) {
			// forward cluster state infomation msg to super broker 
			zmq::message_t msg;
			auto rc = socketBe_.recv(msg, zmq::recv_flags::none);
			socketPush_.send(msg, zmq::send_flags::none);
			//LOG_0 << clusterStateInfo_.clusterName << " ready worker count:" << clusterStateInfo_.readyWorkerCount << "\n";
		}
	}
}
void ClusterStateReporter::setClusterState(const ClusterStateInfo& stateInfo) {
	zmq::message_t msg(sizeof(ClusterStateInfo));
	clusterStateInfo_.readyWorkerCount = stateInfo.readyWorkerCount;
	memcpy(msg.data(), &clusterStateInfo_, sizeof(ClusterStateInfo));
	socketFe_.send(msg, zmq::send_flags::none);
}