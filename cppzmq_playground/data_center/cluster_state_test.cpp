#include "cluster_state.h"

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = 0;
	FLAGS_v = 0;
	FLAGS_log_dir = "./logs";
	google::InitGoogleLogging(argv[0]);

	ClusterState cs1("ClusterState1"), cs2("ClusterState2"), cs3("ClusterState3");
	ClusterStateProxy csProxy;

	csProxy.startTask();

	cs1.startTask();
	cs2.startTask();
	cs3.startTask();

	cs1.wait();
	cs2.wait();
	cs3.wait();

	csProxy.wait();

	return 0;
}