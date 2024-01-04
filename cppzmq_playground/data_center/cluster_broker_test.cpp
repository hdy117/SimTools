#include "cluster_broker.h"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		LOG_ERROR << "usage cluser cluster_name\n";
		return -1;
	}

	//initSpdlog();
	FLAGS_v = 0;
	FLAGS_logtostderr = 1;
	FLAGS_log_dir = "./logs";
	google::InitGoogleLogging(argv[0]);

	ClusterCfg clusterCfg;

	clusterCfg.clusterName = argv[1];
	clusterCfg.localFrontend = constant::kLocal_Frontend_1;
	clusterCfg.localBackend = constant::kLocal_backend_1;

	clusterCfg.superBrokerCfg.superBroker_IP = constant::kSuperBroker_IP;

	OneCluster cluster(clusterCfg);
	ClusterHelper::buildCluster(cluster);

	cluster.startTask();

	cluster.wait();

	//const int clientNum = 5, workerNum = 3;
	//std::vector<ClientPtr> clients;
	//std::vector<WorkerPtr> workers;

	//// create clients
	//for (auto i = 0; i < clientNum; ++i) {
	//	clients.emplace_back(new Client(std::string("stub_" + std::to_string(i))));
	//	clients.at(i)->startTask();
	//}

	//// create workers
	//for (auto i = 0; i < workerNum; ++i) {
	//	workers.emplace_back(new Worker(std::string("worker_" + std::to_string(i))));
	//	workers.at(i)->startTask();
	//}

	//// start local balancer
	//LocalBalanceBroker broker;
	//broker.startTask();

	//// wait until end(forever loop)
	//for (auto& client : clients) {
	//	client->wait();
	//}
	//for (auto& worker : workers) {
	//	worker->wait();
	//}
	//broker.wait();

	return 0;
}