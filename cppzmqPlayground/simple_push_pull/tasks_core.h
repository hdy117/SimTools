#pragma once

#include "zmq.hpp"
#include <string>
#include <iostream>

/**
 * @brief tasks manager, distribute tasks and collect results
 *
 */
class TasksManager {
public:
  TasksManager(const std::string& ventilatorPort = "5556", const std::string& collectPort = "5557");
  virtual ~TasksManager();
  void distributeTasks();
  void collectResults();

private:
  std::string mVentilatorPort, mCollectorPort;

  zmq::context_t mContext;
  zmq::socket_t mSocketVentilator, mSocketCollector;
  size_t mTasksNum;
};

/**
 * @brief weather subscriber
 *
 */
class TaskWorker {
public:
  TaskWorker(const std::string &serverIP, const std::string& ventilatorPort = "5556", 
    const std::string& collectPort = "5557");
  virtual ~TaskWorker();
  void work();

private:
  std::string mServerIP, mVentilatorPort, mCollectorPort;

  zmq::context_t mContext;
  zmq::socket_t mSocketRecvTask, mSocketReportResult;

  std::string mWorkerName;
};