#include <string>
#include <memory>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

#include "weather_core.h"
#include "sim_log.h"

int main() { 
	WeatherPuber puber;
	const size_t CONST_100 = 100;

	std::vector<WeatherInfo> weatherInfo_s;
	weatherInfo_s.reserve(CONST_100);

	LOG_0 << "pub server created.\n";

	for (auto i = 0; i < CONST_100; ++i) {
		weatherInfo_s.emplace_back();

		auto& weatherInfo = weatherInfo_s.at(i);
		if (i & 0x01) {
			memcpy(weatherInfo.mCityName, city_name::BeiJing.c_str(), city_name::BeiJing.size());
			weatherInfo.mCityNameLength = city_name::BeiJing.size() + 1;
			weatherInfo.mCityName[city_name::BeiJing.size() + 1] = '\0';
		}
		else {
			memcpy(weatherInfo.mCityName, city_name::ShangHai.c_str(), city_name::ShangHai.size());
			weatherInfo.mCityNameLength = city_name::ShangHai.size() + 1;
			weatherInfo.mCityName[city_name::ShangHai.size() + 1] = '\0';
		}
		weatherInfo.mTemperature = i / 3.1415926f;
		weatherInfo.mRelhumidity = i * 1.0f;
		
		LOG_0 << "current i:" << i << " in server.\n";
		
		puber.publish(weatherInfo);

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	return 0; 
}