#include "sim_log.h"

#if defined(Use_Spdlog)

std::shared_ptr<spdlog::logger> g_logger;

void initSpdlog() {
  // async log
  spdlog::init_thread_pool(8192, 1);

  // log info pattern
  /*std::string log_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%s:%!:%#]%$ %v");*/
  std::string log_pattern(
      "[%Y-%m-%d %H:%M:%S.%e %l %s::%#]%$ %v");

  // log to console
  spdlog::sink_ptr console_sink =
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern(log_pattern);
  console_sink->set_level(spdlog::level::info);

  // log to file
  spdlog::sink_ptr file_sink =
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "./logs/sim_log.txt", true);
  file_sink->set_level(spdlog::level::info);
  file_sink->set_pattern(log_pattern);

  // create async logger
  std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
  g_logger = std::make_shared<spdlog::async_logger>(
      "async_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
      spdlog::async_overflow_policy::overrun_oldest);

  // set as default logger
  spdlog::set_default_logger(g_logger);

  g_logger->set_level(spdlog::level::info);

  spdlog::flush_every(std::chrono::seconds(3));
}
#endif