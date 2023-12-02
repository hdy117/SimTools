#include <string>
#include <memory>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <random>

#include "tasks_core.h"
#include "sim_log.h"

int main() { 
	TasksManager taskManager;

	taskManager.distributeTasks();

	return 0; 
}