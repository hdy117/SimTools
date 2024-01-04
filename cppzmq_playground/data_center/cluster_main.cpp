#include "cluster_broker.h"

int main(int argc, char *argv[]) {
	if (argc != 3) {
		LOG_ERROR << "usage cluser cluster_name cluster_index\n";
		return -1;
	}

	// glog init
	FLAGS_v = 0;
	FLAGS_logtostderr = 0;
	FLAGS_log_dir = "./logs";
	google::InitGoogleLogging(argv[0]);

	// get index of cluster
	int index = std::atoi(argv[2]);

	// configure cluster
	ClusterCfg clusterCfg;
	clusterCfg.clusterName = std::string(argv[1]) + "_" + std::string(argv[2]);
	clusterCfg.localFrontend = constant::kLocal_Frontend[index];
	clusterCfg.localBackend = constant::kLocal_backend[index];

	// build cluster
	OneCluster cluster(clusterCfg);
	ClusterHelper::buildCluster(cluster);

	// start cluster
	cluster.startTask();

	// wait forever
	cluster.wait();

	return 0;
}