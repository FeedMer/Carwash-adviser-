#include <curl/curl.h>
#include <iostream>
#include <string>
#include "WeatherApi.h"
using namespace std;

size_t MeteoApiConnector::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

MeteoApiConnector::MeteoApiConnector(double longitude, double latitude) {
    this->longitude = longitude;
    this->latitude = latitude;
    curl = curl_easy_init();
}

MeteoApiConnector::~MeteoApiConnector() {
    curl_easy_cleanup(curl);
}

string MeteoApiConnector::makeRequest(int forecast) {
    if (!curl) {
        cerr << "Curl init failed!" << endl;
        return "";
    }

    // TODO: ssl verification fix
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


    string url = LINK +
        "latitude=" + to_string(latitude) +
        "&longitude=" + to_string(longitude) +
        "&daily=temperature_2m_min,precipitation_sum,wind_speed_10m_max,wind_direction_10m_dominant&timezone=Europe/Moscow&forecast_days="
        + to_string(forecast);

    string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        cerr << "connection failed: " << curl_easy_strerror(res) << endl;
        return "";
    }

    return readBuffer;
}

vector<Weather> MeteoApiConnector::getWeathers(int forecast) {
    vector<Weather> weathers(forecast);
    string response = makeRequest(forecast);
    if (response.empty()) {
        cerr << "RESPONSE IS EMPTRY";
        return weathers;
    }
    json j = json::parse(response);
    for (int i = 0; i < forecast; i++) {
        weathers[i].setTime(j["daily"]["time"][i]);
        weathers[i].setTemperature(j["daily"]["temperature_2m_min"][i]);
        weathers[i].setPrecipitation(j["daily"]["precipitation_sum"][i]);
        weathers[i].setWindSpeed(j["daily"]["wind_speed_10m_max"][i]);
        weathers[i].setWindDirection(j["daily"]["wind_direction_10m_dominant"][i]);
    }
    return weathers;
}