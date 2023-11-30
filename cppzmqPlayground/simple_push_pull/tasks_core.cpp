#include "tasks_core.h"
#include "sim_log.h"

#include <thread>
#include <chrono>
#include <random>

/****puber****/

TasksManager::TasksManager(const std::string& ventilatorPort, const std::string& collectPort) : 
  mVentilatorPort(ventilatorPort), mCollectorPort(collectPort) {
  mContext = zmq::context_t(2);
  mTasksNum = 10;
  
  // ventilator that push tasks to workers
  std::string ventilatorAddr = "tcp://0.0.0.0:" + mVentilatorPort;
  mSocketVentilator = zmq::socket_t(mContext, zmq::socket_type::push);
  mSocketVentilator.bind(ventilatorAddr.c_str());
  LOG_0 << "ventilator bind on " << ventilatorAddr << "\n";

  // sink that collects results
  std::string collectorAddr = "tcp://0.0.0.0:" + mCollectorPort;
  mSocketCollector = zmq::socket_t(mContext, zmq::socket_type::pull);
  mSocketCollector.bind(collectorAddr.c_str());
  LOG_0 << "collector bind on " << collectorAddr << "\n";
}
TasksManager::~TasksManager() { mSocketVentilator.close(); mSocketCollector.close(); }
void TasksManager::distributeTasks() {
  for (size_t i = 0; i < mTasksNum; ++i) {
    zmq::message_t msg(sizeof(size_t));

    memcpy(msg.data(), &i, sizeof(size_t));
    mSocketVentilator.send(msg, zmq::send_flags::none);
    LOG_0 << "task sent i:" << i << "\n";
  }
}
void TasksManager::collectResults() {
  size_t sum = 0;
  for (size_t i = 0; i < mTasksNum; ++i) {
    zmq::message_t msg(sizeof(size_t));

    mSocketCollector.recv(msg, zmq::recv_flags::none);

    size_t data;
    memcpy(&data, msg.data(), sizeof(size_t));

    sum += data;
    LOG_0 << "collect data:" << data << ", sum:" << sum << "\n";
  }

  LOG_0 << "collect result sum:" << sum << "\n";
}

/****suber****/

TaskWorker::TaskWorker(const std::string& serverIP, const std::string& ventilatorPort, const std::string& collectPort)
    : mServerIP(serverIP), mVentilatorPort(ventilatorPort), mCollectorPort(collectPort) {
  std::random_device randDevice;
  std::default_random_engine randGen(randDevice());
  std::uniform_int_distribution<int> dist(0, 1000000);
  mWorkerName = std::string("worker_") + std::to_string(dist(randDevice));

  mContext = zmq::context_t(2);
  
  std::string serverAddr = "tcp://" + serverIP + ":" + mVentilatorPort;
  mSocketRecvTask = zmq::socket_t(mContext, zmq::socket_type::pull);
  mSocketRecvTask.connect(serverAddr.c_str());
  LOG_0 << mWorkerName << " connect to " << serverAddr << ".\n";

  serverAddr = "tcp://" + serverIP + ":" + mCollectorPort;
  mSocketReportResult = zmq::socket_t(mContext, zmq::socket_type::push);
  mSocketReportResult.connect(serverAddr.c_str());
  LOG_0 << mWorkerName << " report connect to " << serverAddr << ".\n";
}
TaskWorker::~TaskWorker() { mSocketRecvTask.close(); mSocketReportResult.close(); }
void TaskWorker::work() {
  zmq::message_t msgTask(sizeof(size_t)), msgResult(sizeof(size_t));

  mSocketRecvTask.recv(msgTask, zmq::recv_flags::none);
  size_t data = 0;
  memcpy(&data, msgTask.data(), sizeof(size_t));
  LOG_0 << mWorkerName << " recv task data:" << data << "\n";

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  data = data / 2;
  memcpy(msgResult.data(), &data, sizeof(size_t));
  mSocketReportResult.send(msgResult, zmq::send_flags::none);
  LOG_0 << mWorkerName << " send result data:" << data << "\n";
}