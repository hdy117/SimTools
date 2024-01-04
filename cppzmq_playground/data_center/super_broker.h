#pragma once

#include "cluster_core.h"
#include "cluster_state.h"
#include "cluster_broker.h"

class SuperBroker;
using SuperBrokerPtr = std::shared_ptr<SuperBroker>;

/**
 * @brief push/pull for cluster broker state
*/
class ClusterStateBroker : public AsyncRun {
public:
	ClusterStateBroker(const std::string& pullPort = constant::kPullPort);
	virtual ~ClusterStateBroker();
public:
	void printClusterStateMap();
protected:
	virtual void runTask() override;
protected:
	[[deprecated("do not use this function since this class use push/pull instead of pub/sub")]]
	void subscribe(const std::string& topicPrefix = "ClusterState");
private:
	zmq::context_t context_;
	zmq::socket_t socketPull_;
	std::map<std::string, ClusterStateInfoPtr> clusterInfoMap_;
};

/**
 * @brief super broker
*/
class SuperBroker : public AsyncRun {
public:
	explicit SuperBroker(const SuperBrokerCfg& superBrokerCfg);
	virtual ~SuperBroker();
protected:
	virtual void runTask() override;
private:
	ClusterStateBrokerPtr clusterStateBroker_;
	SuperBrokerCfg superBrokerCfg_;
};
