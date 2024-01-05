#pragma once

#include "cluster_core.h"
#include "cluster_state.h"
#include "cluster_broker.h"

class SuperBroker;
using SuperBrokerPtr = std::shared_ptr<SuperBroker>;

/**
 * @brief super broker
*/
class SuperBroker : public AsyncRun {
public:
	explicit SuperBroker(const SuperBrokerCfg& superBrokerCfg);
	virtual ~SuperBroker();
protected:
	virtual void runTask() override;
	void statePuller();
	void taskRouter();
private:
	ClusterStateBrokerPtr clusterStateBroker_;
	SuperBrokerCfg superBrokerCfg_;
	zmq::context_t context_;

	// task router
	zmq::socket_t socketCloudTask_;

	// pull cluster state info
	zmq::socket_t socketPull_;
	std::map<std::string, ClusterStateInfoPtr> clusterInfoMap_;
};
