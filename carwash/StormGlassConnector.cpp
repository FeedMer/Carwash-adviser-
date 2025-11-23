#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include "WeatherApi.h"

using namespace std;
//using json = nlohmann::json;

size_t StormGlassConnector::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

StormGlassConnector::StormGlassConnector(double longitude, double latitude) {
    this->longitude = longitude;
    this->latitude = latitude;
    curl = curl_easy_init();
}

StormGlassConnector::~StormGlassConnector() {
    curl_easy_cleanup(curl);
}

string StormGlassConnector::makeRequest() {
    if (!curl) {
        cerr << "Curl init failed!" << endl;
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    string url = LINK +
        "?lat=" + to_string(latitude) +
        "&lng=" + to_string(longitude) +
        "&params=airTemperature,windSpeed,windDirection,precipitation";
    // headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: " + API_KEY).c_str());

    string readBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        cerr << "Connection failed: " << curl_easy_strerror(res) << endl;
        return "";
    }

    return readBuffer;
}

Weather StormGlassConnector::getCurrentWeather() {
    Weather w;
    string response = makeRequest();

    if (response.empty()) {
        cerr << "RESPONSE IS EMPTY" << endl;
        return w;
    }

    try {
        json j = json::parse(response);
        w = j["hours"][0].get<Weather>();
    }
    catch (const exception& e) {
        cerr << "JSON parsing error: " << e.what() << endl;
    }

    return w;
}

vector<Weather> StormGlassConnector::getWeatherForFiveDays() {
    vector<Weather> result;
    string response = makeRequest();

    if (response.empty()) {
        cerr << "RESPONSE IS EMPTY" << endl;
        return result;
    }

    try {
        json j = json::parse(response);
        json hours = j["hours"];

        if (hours.empty()) {
            cerr << "No hours data available" << endl;
            return result;
        }

  
        int daysToProcess = min(5, (int)(hours.size() / 24));

        for (int day = 0; day < daysToProcess; day++) {
            Weather maxPrecipitationWeather;
            double maxPrecipitation = -1;

            int startHour = day * 24;
            int endHour = min(startHour + 24, (int)hours.size());

            for (int hour = startHour; hour < endHour; hour++) {
                try {
                    Weather currentWeather = hours[hour].get<Weather>();
                    double precipitation = currentWeather.getPrecipitation();

                    if (precipitation > maxPrecipitation) {
                        maxPrecipitation = precipitation;
                        maxPrecipitationWeather = currentWeather;
                    }
                }
                catch (const exception& e) {
                    cerr << "Error parsing hour " << hour << ": " << e.what() << endl;
                }
            }

            if (maxPrecipitation >= 0) {
                result.push_back(maxPrecipitationWeather);
            }
        }
    }
    catch (const exception& e) {
        cerr << "JSON parsing error: " << e.what() << endl;
    }

    return result;
}
