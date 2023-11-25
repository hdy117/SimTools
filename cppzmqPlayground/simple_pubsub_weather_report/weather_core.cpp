#include "weather_core.h"
#include "sim_log.h"

const uint32_t WeatherInfoSize = sizeof(WeatherInfo);

void printWeatherInfo(const WeatherInfo& weatherInfo) {
  LOG_0 << "city:" << weatherInfo.mCityName 
    << ", city code length:" << weatherInfo.mCityNameLength
    << ", temperature:" << weatherInfo.mTemperature 
    << ", relhumidity:" << weatherInfo.mRelhumidity << ".\n";
}

/****puber****/

WeatherPuber::WeatherPuber(const std::string &port) : mPort(port) {
  mContext = zmq::context_t(2);
  mSocket = zmq::socket_t(mContext, zmq::socket_type::pub);

  std::string bindAddr = "tcp://0.0.0.0:" + mPort;
  mSocket.bind(bindAddr.c_str());
  LOG_0 << "server bind on " << bindAddr << "\n";
}
WeatherPuber::~WeatherPuber() { mSocket.close(); }
size_t WeatherPuber::publish(const WeatherInfo &weatherInfo) {
  printWeatherInfo(weatherInfo);
  zmq::message_t msg(WeatherInfoSize);
  memcpy(msg.data(), &weatherInfo, WeatherInfoSize);
  mSocket.send(msg, zmq::send_flags::none);
}

/****suber****/

WeatherSuber::WeatherSuber(const std::string &serverIP,
                           const std::string &subTopic, const std::string &port)
    : mServerIP(serverIP), mPort(port), mSubTopic(subTopic) {
  mSuberName = subTopic;

  mContext = zmq::context_t(2);
  mSocket = zmq::socket_t(mContext, zmq::socket_type::sub);

  std::string serverAddr = "tcp://" + serverIP + ":" + port;

  mSocket.connect(serverAddr.c_str());
  LOG_0 << "client connect to " << serverAddr << ".\n";

  setSubTopic(subTopic);
}
WeatherSuber::~WeatherSuber() { mSocket.close(); }
void WeatherSuber::setSubTopic(const std::string &subTopic) {
  mSubTopic = subTopic;
  mSocket.setsockopt(ZMQ_SUBSCRIBE, mSubTopic.c_str(), mSubTopic.size());
  LOG_0 << "subscribe to " << mSubTopic << ".\n";
}
size_t WeatherSuber::subscribe() {
  zmq::message_t msg;
  mSocket.recv(msg, zmq::recv_flags::none);

  WeatherInfo weatherInfo;
  memcpy(&weatherInfo, msg.data(), WeatherInfoSize);
  
  LOG_0 << "mSuberName:" << mSuberName << ".\n";
  
  printWeatherInfo(weatherInfo);

  return 0;
}