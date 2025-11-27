#include <string>
#include <json.hpp>
#include "WeatherApi.h"
using namespace std;
using json = nlohmann::json;

double Weather::getTemperature() {
	return temperature;
}

int Weather::getWindDirection() {
	return windDirection;
}

double Weather::getWindSpeed() {
	return windSpeed;
}

double Weather::getPrecipitation() {
	return precipitation;
}

string Weather::getTime() {
	return time;
}

void Weather::setTime(string time) {
	this->time = time;
}

void Weather::setPrecipitation(double precipitation) {
	this->precipitation = precipitation;
}

void Weather::setTemperature(double temperature) {
	this->temperature = temperature;
}

void Weather::setWindSpeed(double windSpeed) {
	this->windSpeed = windSpeed;
}

void Weather::setWindDirection(int windDirection) {
	this->windDirection = windDirection;
}

ostream& operator<<(ostream& stream, Weather& w) {
	stream << w.time << '\n'
		<< w.temperature << " celsius\n"
		<< w.windSpeed << " km/h speed, " << w.windDirection
		<< endl;
	return stream;
}

void from_json(const json& j, Weather& w) {
	j.at("time").get_to(w.time);
	j.at("temperature_2m").get_to(w.temperature);
	j.at("wind_speed_10m").get_to(w.windSpeed);
	j.at("wind_direction_10m").get_to(w.windDirection);
	j.at("precipitation").get_to(w.precipitation);
}