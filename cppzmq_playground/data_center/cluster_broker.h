#pragma once

#include "cluster_core.h"
#include "cluster_state.h"

class Client;
using ClientPtr = std::shared_ptr<Client>;

class Worker;
using WorkerPtr = std::shared_ptr<Worker>;

class LocalBalanceBroker;
using LocalBalanceBrokerPtr = std::shared_ptr<LocalBalanceBroker>;

class Client : public AsyncRun{
public:
	explicit Client(const std::string& id, const std::string& port = constant::kLocal_Frontend[0]);
	virtual ~Client();
public:
	const std::string& getID() { return id_; }
	void genTask(TaskMeta& meta, TaskRequest& task);
protected:
	virtual void runTask() override;
	void sendRequest();
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::string id_;
};

class Worker:public AsyncRun {
public:
	explicit Worker(const std::string& id, const std::string& port = constant::kLocal_backend[0]);
	virtual ~Worker();
public:
	const std::string& getID() { return id_; }
protected:
	virtual void runTask() override;
	void process();
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::string id_;
	std::atomic<uint32_t> workCounter_;
};

class LocalBalanceBroker : public AsyncRun{
public:
	LocalBalanceBroker(const ClusterCfg& clusterCfg);
	virtual ~LocalBalanceBroker();
public:
	uint32_t getReadyWorkerCount();
protected:
	/**
	 * @brief load balancer
	*/
	virtual void runTask() override;
	/**
	 * @brief route reply message from local worker
	*/
	void routeReply();
	/**
	 * @brief router request message from local client
	*/
	void routeRequest();
	/**
	 * @brief route request or reply from super broker
	*/
	void routeCloud();
private:
	// contect from cluster
	zmq::context_t context_;
	// socket to communicate with local client
	zmq::socket_t socketFrontEnd_; 
	// socket to communicate with local worker
	zmq::socket_t socketBackEnd_;
	// socket to communicate with socketFrontEnd_ and socketCloudFe_
	zmq::socket_t socketCloudTask_;
	// queue of ready workers, used for local load balance
	std::queue<std::string> workReadyQueue_;
	// ready worker counter, used to be reported to super broker, 
	// so that super broker can router tasks to this cluster
	std::atomic<uint32_t> readyWorkerCount_;
	// cluster name
	std::string clusterName_;
	// cluster config
	ClusterCfg clusterCfg_;
};

/**
 * @brief one cluster
*/
class OneCluster : public AsyncRun {
public:
	explicit OneCluster(const ClusterCfg& clusterCfg);
	virtual ~OneCluster();
public:
	const ClusterCfg& getClusterCfg() { return clusterCfg_; }
	std::vector<WorkerPtr>& getWorkers() { return workers_; }
	std::vector<ClientPtr>& getClients() { return clients_; }
	const std::string& getClusterName() { return clusterName_; }
	uint32_t getReadyWorkerCount();
protected:
	/**
	 * @brief forward task and reply between local cluster or super broker
	*/
	virtual void runTask() override;
private:
	ClusterCfg clusterCfg_;
	std::vector<WorkerPtr> workers_;
	std::vector<ClientPtr> clients_;
	std::string clusterName_;
	LocalBalanceBrokerPtr localBalancer_;
	ClusterStateInfo stateInfo_;
	ClusterStateReporterPtr clusterState_;
};

class ClusterHelper {
public:
	static void buildCluster(OneCluster& cluster);
};