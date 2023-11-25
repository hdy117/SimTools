#pragma once

#include "zmq.hpp"
#include <string>


namespace city_name {
const std::string BeiJing = "BeiJing";
const std::string ShangHai = "ShangHai";
} // namespace city_name

/**
 * @brief weather information
*/
struct WeatherInfo {
  char mCityName[256];
  uint32_t mCityNameLength;
  float mTemperature;
  float mRelhumidity;
};

void printWeatherInfo(const WeatherInfo& weatherInfo);

extern const uint32_t WeatherInfoSize;

/**
 * @brief weather publisher
 *
 */
class WeatherPuber {
public:
  WeatherPuber(const std::string &port = "5555");
  virtual ~WeatherPuber();
  size_t publish(const WeatherInfo &weatherInfo);

private:
  std::string mPort;

  zmq::context_t mContext;
  zmq::socket_t mSocket;
};

/**
 * @brief weather subscriber
 *
 */
class WeatherSuber {
public:
  WeatherSuber(const std::string &serverIP,
               const std::string &subTopic = city_name::BeiJing,
               const std::string &port = "5555");
  virtual ~WeatherSuber();
  void setSubTopic(const std::string &subTopic);
  size_t subscribe();

private:
  std::string mServerIP, mPort;
  std::string mSubTopic;
  std::string mSuberName;

  zmq::context_t mContext;
  zmq::socket_t mSocket;
};