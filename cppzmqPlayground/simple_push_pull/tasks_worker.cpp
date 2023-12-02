#include "tasks_core.h"

int main() { 
	TaskWorker worker(std::string("172.18.224.1"));

	while (true) {
		worker.work();
	}

	return 0; 
}