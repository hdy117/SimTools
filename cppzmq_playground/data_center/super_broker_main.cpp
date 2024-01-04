#include "super_broker.h"

int main() {
	SuperBrokerCfg superBrokerCfg;
	SuperBroker superBroker(superBrokerCfg);

	superBroker.startTask();

	superBroker.wait();
}