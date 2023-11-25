#include "weather_core.h"

int main() { 
	WeatherSuber suberBeijing("172.18.224.1", city_name::BeiJing);

	while (true) {
		suberBeijing.subscribe();
	}

	return 0; 
}