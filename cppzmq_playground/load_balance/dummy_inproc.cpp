#include <string>
#include <vector>
#include <map>
#include <thread>
#include <functional>

#include "sim_log.h"

#include "zmq.hpp"

#include "message/location.pb.h"
#include "message/basic.pb.h"
#include "message/trajectory.pb.h"


#if defined(_WIN32)
#pragma comment(lib, "libzmq-mt-4_3_5.lib")
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "glog.lib")
#pragma comment(lib, "gflags.lib")
#endif

class ThreadSignal {
public:
	ThreadSignal(zmq::context_t* context) :context_(context) {
		socket_ = zmq::socket_t(*context_, zmq::socket_type::pair);
		socket_.connect("inproc://signal_a");
	}
	virtual ~ThreadSignal() {
		if(handle_.joinable()) handle_.join();
		socket_.close();
		LOG_0 << "signal: fire signal object quit.\n";
	}
public:
	void startThread() {
		handle_ = std::thread(&ThreadSignal::fireSignal_A, this);
	}
private:
	void fireSignal_A() {
		LOG_0 << "signal: sleep 5 seconds, then fire signal.\n";
		std::this_thread::sleep_for(std::chrono::seconds(5));
		zmq::message_t msg(1);
		memcpy(msg.data(), "", 1);
		zmq::message_t msgZero(0);
		auto result = socket_.send(msgZero, zmq::send_flags::none);
		LOG_0 << "signal: signal and quit, send status:" << result.value() << "\n";
	}
private:
	zmq::context_t* context_;
	zmq::socket_t socket_;
	std::thread handle_;
};

class ThreadWait {
public:
	ThreadWait(zmq::context_t* context) :context_(context) {
		socket_ = zmq::socket_t(*context_, zmq::socket_type::pair);
		socket_.bind("inproc://signal_a");
	}
	virtual ~ThreadWait() {
		socket_.close();
	}
public:
	void wait() {
		zmq::message_t msg;
		
		LOG_0 << "wait: waiting ...\n";
		socket_.recv(msg, zmq::recv_flags::none);
		LOG_0 << "wait: got you ...\n";
	}
private:
	zmq::context_t* context_;
	zmq::socket_t socket_;
};

int main() {
	zmq::context_t context(1);
	ThreadWait wait(&context);
	ThreadSignal signal(&context);

	signal.startThread();
	wait.wait();

	return 0;
}