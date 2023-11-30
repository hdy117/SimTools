#include "tasks_core.h"

int main() { 
	using TaskWorkerPtr = std::shared_ptr<TaskWorker>;
	std::vector<TaskWorkerPtr> workers;

	const size_t WorkerNum = 3;

	for (auto i = 0; i < WorkerNum; ++i) {
		workers.emplace_back(std::make_shared<TaskWorker>(std::string("172.18.224.1")));
	}
	while (true) {
		for (auto i = 0; i < WorkerNum; ++i) {
			auto worker = workers.at(i);
			worker->work();
		}
	}

	return 0; 
}