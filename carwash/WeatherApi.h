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

    void setTime(string time);
    void setTemperature(double temperature);
    void setWindSpeed(double windSpeed);
    void setWindDirection(int windDirection);
    void setPrecipitation(double precipitation);

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

class MeteoblueConnector {
private:
    std::string LINK = "https://my.meteoblue.com/packages/basic-day?apikey=";
    string API_KEY = "";
    double longitude;
    double latitude;
    CURL* curl;
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);

public:
    MeteoblueConnector(double longitude, double latitude);
    ~MeteoblueConnector();
    string makeRequestOfSomeWeathers(int forecast);
    vector<Weather> getSomeWeather(int forecast);
};