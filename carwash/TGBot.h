#pragma once
#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <thread>
#include <chrono>
#include <set>
#include <ctime>
#include "DataBase.h"
#include "DeepseekAPI.h"

using json = nlohmann::json;

class TelegramBot {
private:
    DataBase db;
    DeepseekAPI ds;
    std::string bot_token;
    std::string api_url;
    bool running;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t totalSize = size * nmemb;
        output->append((char*)contents, totalSize);
        return totalSize;
    }

    std::string curlGet(const std::string& url) {
        CURL* curl = curl_easy_init();
        std::string response;
        if (curl) {
            // НАСТРОЙКИ SSL
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключить проверку сертификата
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Отключить проверку хоста

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            auto res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return response;
    }

    void curlPost(const std::string& url, const std::string& data) {
        CURL* curl = curl_easy_init();
        if (curl) {
            // НАСТРОЙКИ SSL
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключить проверку сертификата
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Отключить проверку хоста

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            auto res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
    }

    json locationRequestKeyboard() {
        return {
            {"keyboard", json::array({
                json::array({
                    {
                        {"text", u8"Отправить местоположение ??"},
                        {"request_location", true}
                    }
                })
            })},
            {"resize_keyboard", true},
            {"one_time_keyboard", true}
        };
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
        cout << "Запуск цикла" << endl;
        const int TARGET_DAY = 1;
        bool sentToday = false;
        DeepseekAPI ds2;

        while (running) {
            //time_t now = time(nullptr);
            //tm localTime;
            //localtime_s(&localTime, &now);
            //int weekday = localTime.tm_wday;
            //int hour = localTime.tm_hour;
            //int minute = localTime.tm_min;

            //if (hour == 0 && minute == 0)
            //    sentToday = false;

            //if (!sentToday) {  // тест уведомлений
            //if (!sentToday && weekday == TARGET_DAY && hour == 9 && minute == 00) {
            //    for (auto chatId : db.usersForMailing()) {
            //        //sendMessage(chatId.telegramId, u8"Напоминание! Сегодня время помыть машину");
            //    }
            //    sentToday = true;
            //}

            auto users = db.usersForMailing();
            if (users.size() > 0) {
                ds2.main();
                if (ds2.toWash) {
                    for (auto user : users) {
                        db.addMessage(user.telegramId, win1251_to_utf8(ds2.prompt), win1251_to_utf8(ds2.result));
                        sendMessage(stoll(user.telegramId), win1251_to_utf8(ds2.result));
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(100));
        }
    }

    void processUpdates() {
        long long lastUpdateId = 0;
        cout << "Запуск цикла" << endl;

        while (running) {
            std::string updatesResponse = curlGet(api_url + "getUpdates?offset=" + std::to_string(lastUpdateId + 1));
            if (updatesResponse.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            json updates;
            try {
                updates = json::parse(updatesResponse);
            }
            catch (...) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            for (auto& update : updates["result"]) {
                lastUpdateId = update["update_id"].get<long long>();
                if (!update.contains("message")) continue;

                auto message = update["message"];
                long long chatId = message["chat"]["id"].get<long long>();
                std::string text = message.contains("text") ? message["text"].get<std::string>() : "";
                
                std::string username = "";
                std::string firstName = "";
                if (message["from"].contains("username") && !message["from"]["username"].is_null()) {
                    username = message["from"]["username"].get<std::string>();
                }
                if (message["from"].contains("first_name") && !message["from"]["first_name"].is_null()) {
                    firstName = message["from"]["first_name"].get<std::string>();
                }
                std::string userNameForDb = firstName;
                if (!username.empty()) {
                    if (!userNameForDb.empty()) userNameForDb += " (@" + username + ")";
                    else userNameForDb = "@" + username;
                }
                if (userNameForDb.empty()) userNameForDb = "User_" + std::to_string(chatId);

                //text = utf8_to_win1251(text);
                cout << endl << "Текст сообщения пользователя: " + utf8_to_win1251(text) << endl;

                if (text == "/start") {
                    db.addTelegramUser(std::to_string(chatId), userNameForDb);
                    sendMessage(chatId, u8"Привет! Мне нужно узнать твоё местоположение:", locationRequestKeyboard());
                }
                else if (message.contains("location")) {
                    sendMessage(chatId, u8"Спасибо! Местоположение сохранено.", mainMenu());
                }
                else if (text == u8"Стоит ли мыть сегодня?") {
                    ds.main();
                    db.addMessage(to_string(chatId), win1251_to_utf8(ds.prompt), win1251_to_utf8(ds.result));
                    sendMessage(chatId, win1251_to_utf8( ds.result), mainMenu());
                }
                else if (text == u8"Я помыл машину") {
                    sendMessage(chatId, u8"Окей!", mainMenu());
                }
                else if (text == u8"Отписаться от уведомлений") {
                    if (db.setUserStatus(std::to_string(chatId), 0))
                        sendMessage(chatId, u8"Хорошо, уведомления временно отключены.", mainMenu());
                }
                else if (text == u8"Настройки") {
                    sendMessage(chatId, u8"Введите новый город или отправьте геолокацию.", locationRequestKeyboard());
                }
                else if (!text.empty()) {
                    ds.texting(text);
                    sendMessage(chatId, ds.result, mainMenu());
                }
                // Этот код слишком сомнительный, его надо переделать
                else if (!text.empty()) {
                    sendMessage(chatId, u8"Спасибо! Город сохранён.", mainMenu());
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

public:
    TelegramBot(const std::string& token) : bot_token(token), running(false) {
        api_url = "https://api.telegram.org/bot" + bot_token + "/";
    }

    ~TelegramBot() {
        stop();
    }

    void sendMessage(long long chatId, const std::string& text, const json& replyMarkup = nullptr) {
        std::string url = api_url + "sendMessage";
        json payload = {
            {"chat_id", chatId},
            {"text", text}
        };
        if (!replyMarkup.is_null())
            payload["reply_markup"] = replyMarkup;

        CURL* curl = curl_easy_init();
        if (curl) {
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            
            // НАСТРОЙКИ SSL
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключить проверку сертификата
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Отключить проверку хоста

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            auto res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
    }

    void start() {
        running = true;
        std::thread notifyThread(&TelegramBot::notificationThread, this);
        notifyThread.detach();
        processUpdates();
    }

    void stop() {
        running = false;
    }
};

//void main() {
//    SetConsoleOutputCP(CP_UTF8);
//    SetConsoleCP(CP_UTF8);
//    std::cout << "Bot started..." << std::endl;
//
//    TelegramBot bot("8212512135:AAFFT4JdYLPXnYQrM_EIJ2EF886LBPEqXdI");
//    bot.start();
//}
