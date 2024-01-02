#pragma once

#include "cluster_core.h"

class ClusterState : public AsyncRun{
public:
	ClusterState(const std::string& clusterName, const std::string& xpubxsubIP = "127.0.0.1", const std::string& xpubPort = constant::kXPubPort, const std::string& xsubPort = constant::kXSubPort);
	virtual ~ClusterState();
	virtual void runTask() override;
public:
	inline const std::string& getClusterName() const { return clusterName_; }
private:
	zmq::context_t context_;
	zmq::socket_t socketStatePub_;
	zmq::socket_t socketStateSub_;
	std::string xpubxsubIP_, xpubPort_, xsubPort_;
	std::string clusterName_;
};