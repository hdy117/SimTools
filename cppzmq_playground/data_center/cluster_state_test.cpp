#include "cluster_state.h"
#include "super_broker.h"

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = 1;
	FLAGS_v = 0;
	FLAGS_log_dir = "./logs";
	google::InitGoogleLogging(argv[0]);

	ClusterStateReporter cs1("ClusterState1"), cs2("ClusterState2"), cs3("ClusterState3");
	ClusterStateBroker csProxy;

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