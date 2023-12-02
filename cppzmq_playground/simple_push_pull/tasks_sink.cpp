#include <chrono>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>


#include "sim_log.h"
#include "tasks_core.h"


int main() {
  TaskCollector taskCollector;

  taskCollector.collectResults();

  return 0;
}