#pragma once

#include "zmq.hpp"
#include <string>
#include <iostream>

extern const size_t GlobalTaskNum;

/**
 * @brief tasks manager, distribute tasks and collect results
 *
 */
class TasksManager {
public:
  TasksManager(const std::string& ventilatorPort = "5556");
  virtual ~TasksManager();
  void distributeTasks();
private:
  std::string mVentilatorPort;

  zmq::context_t mContext;
  zmq::socket_t mSocketVentilator;
};

class TaskCollector {
public:
  TaskCollector(const std::string& collectPort = "5557");
  virtual ~TaskCollector();
  void collectResults();

private:
  std::string mCollectorPort;

  zmq::context_t mContext;
  zmq::socket_t mSocketCollector;
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