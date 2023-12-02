#include "weather_core.h"

int main() {
	WeatherSuber suberShangHai("172.18.224.1", city_name::ShangHai);

	while (true) {
		suberShangHai.subscribe();
	}

	return 0;
}