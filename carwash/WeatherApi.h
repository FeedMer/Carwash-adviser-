#pragma once
#include <string>
#include <curl/curl.h>
#include <json.hpp>
using namespace std;
using json = nlohmann::json;

class Weather {
	string time;
	double temperature;
	double windSpeed;
	int windDirection;
	double precipitation;
public:
	friend void from_json(const json& j, Weather& w);			//deserialisation
	friend ostream& operator<<(ostream& stream, Weather& w);

	string getTime();
	double getTemperature();
	double getWindSpeed();
	double getPrecipitation();
	int getWindDirection();
};

class MeteoApiConnector {
private:
    string LINK = "https://api.open-meteo.com/v1/forecast?";    // meteo api base url
    double longitude;
    double latitude;
    CURL* curl;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

public:
    MeteoApiConnector(double longitude, double latitude);
    ~MeteoApiConnector();

    string makeRequest();
	Weather getCurrentWeather();
};