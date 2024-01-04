#pragma once

#include "cluster_core.h"

class ClusterStateReporter;
using ClusterStateReporterPtr = std::shared_ptr<ClusterStateReporter>;

class ClusterStateReporter : public AsyncRun{
public:
	ClusterStateReporter(const std::string& clusterName, const std::string& pullerIP = constant::kSuperBroker_IP, 
		const std::string& pullPort = constant::kSuperBroker_PullStatePort);
	virtual ~ClusterStateReporter();
protected:
	virtual void runTask() override;
public:
	inline const std::string& getClusterName() const { return clusterName_; }
	void setClusterState(const ClusterStateInfo& stateInfo);
private:
	zmq::context_t context_;
	// socket for inter-thread communication, socketFe_ is main thread
	zmq::socket_t socketFe_;
	// socket for inter-thread communication, socketBe_ is backend thread
	zmq::socket_t socketBe_;
	// push peer with puller in super broker
	zmq::socket_t socketPush_;
	// puller ip and port in super broker
	std::string pullerIP_, pullPort_;
	// cluster name and state info
	std::string clusterName_;
	ClusterStateInfo clusterStateInfo_;
};