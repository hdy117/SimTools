#include "super_broker.h"

int main(int argc, char *argv[]) {
	// glog init
	FLAGS_v = 0;
	FLAGS_logtostderr = 1;
	FLAGS_logtostdout = 1;
	google::InitGoogleLogging(argv[0]);
	FLAGS_log_dir = "./logs";

	SuperBrokerCfg superBrokerCfg;
	SuperBroker superBroker(superBrokerCfg);

	superBroker.startTask();

	superBroker.wait();
}