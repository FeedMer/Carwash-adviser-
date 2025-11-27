#include "WeatherApi.h"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

size_t MeteoblueConnector::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

MeteoblueConnector::MeteoblueConnector(double longitude, double latitude)
    : longitude(longitude), latitude(latitude) {
    ifstream filekey("weather-key.txt");
    filekey >> API_KEY;
    filekey.close();
    curl = curl_easy_init();
}

MeteoblueConnector::~MeteoblueConnector() {
    curl_easy_cleanup(curl);
}

string MeteoblueConnector::makeRequest(int forecast) {
    if (!curl) {
        cerr << "Curl init failed!" << endl;
        return "";
    }

    // TODO: ssl verification fix
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


    string url = LINK + API_KEY
        + "&lat=" + to_string(latitude)
        + "&lon=" + to_string(longitude)
        + "&forecast_days=" + to_string(forecast);

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

vector<Weather> MeteoblueConnector::getWeathers(int forecast) {
    string response = makeRequest(forecast);
    vector<Weather> weathers(forecast);
    if (response.empty()) {
        cerr << "RESPONSE IS EMPTY" << endl;
        return weathers;
    }
    json j = json::parse(response);
    for (int i = 0; i < forecast; i++) {
        weathers[i] = Weather();
        weathers[i].setTime(j["data_day"]["time"][i]);
        weathers[i].setPrecipitation(j["data_day"]["precipitation"][i]);
        weathers[i].setTemperature(j["data_day"]["temperature_min"][i]);
        weathers[i].setWindDirection((int)j["data_day"]["winddirection"][i]);
        weathers[i].setWindSpeed(j["data_day"]["windspeed_max"][i]);
    }
    return weathers;
}