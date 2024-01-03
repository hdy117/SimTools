#pragma once

#include "cluster_core.h"

class ClusterState : public AsyncRun{
public:
	ClusterState(const std::string& clusterName, const std::string& pullerIP = "127.0.0.1", 
		const std::string& pullPort = constant::kPullPort);
	virtual ~ClusterState();
protected:
	virtual void runTask() override;
public:
	inline const std::string& getClusterName() const { return clusterName_; }
	const ClusterStateInfo& getClusterStateInfo() const { return clusterStateInfo_; };
private:
	zmq::context_t context_;
	zmq::socket_t socketPush_;
	std::string pullerIP_, pullPort_;
	std::string clusterName_;
	ClusterStateInfo clusterStateInfo_;
};