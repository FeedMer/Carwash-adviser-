#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <set>
#include <ctime>

using json = nlohmann::json;

using namespace std;
set<long long> subscribers; //вр. бд
const string BOT_TOKEN = "TOKEN";
const string API_URL = "https://api.telegram.org/bot" + BOT_TOKEN + "/";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

string curlGet(const string& url) {
    CURL* curl = curl_easy_init();
    string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

void curlPost(const string& url, const string& data) {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

json locationRequestKeyboard() {
    return {
        {"keyboard", json::array({
            json::array({
                {
                    {"text", u8"Отправить местоположение 📍"},
                    {"request_location", true}
                }
            })
        })},
        {"resize_keyboard", true},
        {"one_time_keyboard", true}
    };
}


void sendMessage(long long chatId, const string& text, const json& replyMarkup = nullptr) {
    string url = API_URL + "sendMessage";
    json payload = {
        {"chat_id", chatId},
        {"text", text}
    };
    if (!replyMarkup.is_null())
        payload["reply_markup"] = replyMarkup;

    CURL* curl = curl_easy_init();
    if (curl) {
        string payloadStr = payload.dump();
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

json mainMenu() {
    return {
        {"keyboard", json::array({
            json::array({ u8"Стоит ли мыть сегодня?" }),
            json::array({ u8"Я помыл машину" }),
            json::array({ u8"Отписаться от уведомлений" }),
            json::array({ u8"Настройки" })
        })},
        {"resize_keyboard", true}
    };
}

void notificationThread() {
    const int TARGET_DAY = 5;
    bool sentToday = false;

    while (true) {
        time_t now = time(nullptr);
        tm localTime;
        localtime_s(&localTime, &now);
        int weekday = localTime.tm_wday;
        int hour = localTime.tm_hour;
        int minute = localTime.tm_min;
        if (hour == 0 && minute == 0)
            sentToday = false;
        //if (!sentToday) тест уведомлений{
        if (!sentToday && weekday == TARGET_DAY && hour == 9 && minute == 00)
            for (long long chatId : subscribers) {
                sendMessage(chatId, u8"Напоминание! Сегодня время помыть машину");
            }
        sentToday = true;
    }

    this_thread::sleep_for(chrono::seconds(30));
}
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    cout << "Bot started..." << endl;

    thread notifyThread(notificationThread);
    notifyThread.detach();

    long long lastUpdateId = 0;

    while (true) {
        string updatesResponse = curlGet(API_URL + "getUpdates?offset=" + to_string(lastUpdateId + 1));
        if (updatesResponse.empty()) {
            this_thread::sleep_for(chrono::seconds(2));
            continue;
        }

        json updates;
        try {
            updates = json::parse(updatesResponse);
        }
        catch (...) {
            this_thread::sleep_for(chrono::seconds(2));
            continue;
        }

        for (auto& update : updates["result"]) {
            lastUpdateId = update["update_id"].get<long long>();
            if (!update.contains("message")) continue;
            auto message = update["message"];
            long long chatId = message["chat"]["id"].get<long long>();
            string text = message.contains("text") ? message["text"].get<string>() : "";

            if (text == "/start") {
                subscribers.insert(chatId);
                sendMessage(chatId, u8"Привет! Мне нужно узнать твоё местоположение:", locationRequestKeyboard());
                //cout << "Added subscriber: " << chatId << endl; проверка что пользователь добавлен
            }
            else if (message.contains("location")) {
                sendMessage(chatId, u8"Спасибо! Местоположение сохранено.", mainMenu());
            }
            else if (text == u8"Стоит ли мыть сегодня?") {
                sendMessage(chatId, u8"Пока не сделано", mainMenu());
            }
            else if (text == u8"Я помыл машину") {
                sendMessage(chatId, u8"Окей!", mainMenu());
            }
            else if (text == u8"Отписаться от уведомлений") {
                sendMessage(chatId, u8"Хорошо, уведомления временно отключены.", mainMenu());
                subscribers.erase(chatId);
            }
            else if (text == u8"Настройки") {
                sendMessage(chatId, u8"Введите новый город или отправьте геолокацию.", locationRequestKeyboard());
            }
            else if (!text.empty()) {
                sendMessage(chatId, u8"Спасибо! Город сохранён.", mainMenu());
            }
        }

        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}

