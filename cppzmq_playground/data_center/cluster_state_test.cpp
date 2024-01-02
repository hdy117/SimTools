#include "cluster_state.h"

int main(int argc, char *argv[]) {
	ClusterState cs1("CS1"), cs2("CS2"), cs3("CS3");
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