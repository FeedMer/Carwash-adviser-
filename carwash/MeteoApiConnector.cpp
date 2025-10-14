#include "WeatherApi.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
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

string MeteoApiConnector::makeRequest() {
    if (!curl) {
        cerr << "Curl init failed!" << endl;
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


    string url = LINK +
        "latitude=" + to_string(latitude) +
        "&longitude=" + to_string(longitude) +
        "&current=temperature_2m,wind_speed_10m,wind_direction_10m,precipitation";

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



