#pragma once

#include "cluster_core.h"
#include "cluster_state.h"
#include "cluster_broker.h"

class SuperBroker;
using SuperBrokerPtr = std::shared_ptr<SuperBroker>;

class SuperBroker : public AsyncRun {
public:
	explicit SuperBroker();
	virtual ~SuperBroker();
protected:
	virtual void runTask() override;
private:
	ClusterStateProxyPtr clusterStateBroker_;
};
