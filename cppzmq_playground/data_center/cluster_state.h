#pragma once

#include "cluster_core.h"

class ClusterState;
using ClusterStatePtr = std::shared_ptr<ClusterState>;

class ClusterState : public AsyncRun{
public:
	ClusterState(const std::string& clusterName, const std::string& pullerIP = constant::kSuperBroker_IP, 
		const std::string& pullPort = constant::kPullPort);
	virtual ~ClusterState();
protected:
	virtual void runTask() override;
public:
	inline const std::string& getClusterName() const { return clusterName_; }
	void setClusterState(const ClusterStateInfo& stateInfo);
private:
	zmq::context_t context_;
	zmq::socket_t socketPush_;
	std::string pullerIP_, pullPort_;
	std::string clusterName_;
	ClusterStateInfo clusterStateInfo_;
	std::mutex clusterStateLock_;
};