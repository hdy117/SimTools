#include "tasks_core.h"

int main() { 
	TaskWorker worker(std::string("127.0.0.1"));

	while (true) {
		worker.work();
	}

	return 0; 
}