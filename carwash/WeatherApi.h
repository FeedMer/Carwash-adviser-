#pragma once
#include <string>
#include <curl/curl.h>
using namespace std;

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
};

