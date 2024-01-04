#include "cluster_broker.h"

int main() {
	initSpdlog();

	const int clientNum = 5, workerNum = 3;
	std::vector<ClientPtr> clients;
	std::vector<WorkerPtr> workers;

	// create clients
	for (auto i = 0; i < clientNum; ++i) {
		clients.emplace_back(new Client(std::string("stub_" + std::to_string(i))));
		clients.at(i)->startTask();
	}

	// create workers
	for (auto i = 0; i < workerNum; ++i) {
		workers.emplace_back(new Worker(std::string("worker_" + std::to_string(i))));
		workers.at(i)->startTask();
	}

	// start local balancer
	LocalBalanceBroker broker;
	broker.startTask();

	// wait until end(forever loop)
	for (auto& client : clients) {
		client->wait();
	}
	for (auto& worker : workers) {
		worker->wait();
	}
	broker.wait();

	return 0;
}