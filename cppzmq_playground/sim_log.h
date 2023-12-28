#pragma once

//#define Use_Glog 1
//#define Use_Spdlog 1

#ifdef Use_Glog
#include "glog/logging.h"

#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR LOG(ERROR)
#define LOG_0 VLOG(0)
#define LOG_1 VLOG(1)
#define LOG_2 VLOG(2)
#else
#include <iostream>

#define LOG_0 std::cout
#define LOG_1 std::cout
#define LOG_2 std::cout
#define LOG_WARNING std::cerr
#define LOG_ERROR std::cerr
#endif

#if defined(Use_Spdlog)

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

extern std::shared_ptr<spdlog::logger> g_logger;

#ifdef _WIN32
#pragma comment(lib, "spdlog.lib")
#endif // _WIN32

void initSpdlog();
#endif

