#pragma once

#include "cluster_core.h"

class Client;
using ClientPtr = std::shared_ptr<Client>;

class Worker;
using WorkerPtr = std::shared_ptr<Worker>;

class LocalBalanceBroker;
using LocalBalanceBrokerPtr = std::shared_ptr<LocalBalanceBroker>;

class Client : public AsyncRun{
public:
	explicit Client(const std::string& id, const std::string& port = constant::kLocalFrontend);
	virtual ~Client();
public:
	const std::string& getID() { return id_; }
	void genTask(Task& task);
protected:
	virtual void runTask() override;
	void sendRequest();
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::string id_;
};

class Worker:public AsyncRun {
public:
	explicit Worker(const std::string& id, const std::string& port = constant::kLocalbackend);
	virtual ~Worker();
public:
	const std::string& getID() { return id_; }
protected:
	virtual void runTask() override;
	void processImp();
private:
	zmq::context_t context_;
	zmq::socket_t socket_;
	std::string id_;
	std::atomic<uint32_t> workCounter_;
};

class LocalBalanceBroker : public AsyncRun{
public:
	LocalBalanceBroker(const std::string& portFront = constant::kLocalFrontend, const std::string& portBack = constant::kLocalbackend);
	virtual ~LocalBalanceBroker();
protected:
	virtual void runTask() override;
private:
	zmq::context_t context_;
	zmq::socket_t socketFrontEnd_, socketBackEnd_;
	std::string portFront_, portBack_;
	std::queue<std::string> workReadyQueue_;
};