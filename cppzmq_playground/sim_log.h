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

#elif defined(Use_Spdlog)

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

std::shared_ptr<spdlog::logger> g_logger;

void init_spdlog() {
  // async log
  spdlog::init_thread_pool(8192, 1);

  // log info pattern
  std::string log_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [%l%$] [thread %t] [%s:%!:%#] %v");

  // log to console
  spdlog::sink_ptr console_sink =
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern(log_pattern);
  console_sink->set_level(spdlog::level::info);

  // log to file
  spdlog::sink_ptr file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(
          "./logs/toy_node_a.log");
  file_sink->set_level(spdlog::level::info);
  file_sink->set_pattern(log_pattern);

  // create async logger
  std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
  g_logger = std::make_shared<spdlog::async_logger>(
      "async_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);

  // set as default logger
  spdlog::set_default_logger(g_logger);

  g_logger->set_level(spdlog::level::info);

  spdlog::flush_every(std::chrono::seconds(3));
}
#else
#include <iostream>
#define LOG_0 std::cout
#define LOG_1 std::cout
#define LOG_2 std::cout
#define LOG_WARNING std::cerr
#define LOG_ERROR std::cerr
#endif