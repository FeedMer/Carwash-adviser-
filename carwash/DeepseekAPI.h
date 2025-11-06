#pragma once
#include <string>
#include <json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;
using namespace std;

// Конвертация из Windows-1251 в UTF-8
static std::string win1251_to_utf8(const std::string& win1251_str) {
    int wlen = MultiByteToWideChar(1251, 0, win1251_str.c_str(), -1, NULL, 0);
    if (wlen == 0) return win1251_str;

    std::wstring wide_str(wlen, 0);
    MultiByteToWideChar(1251, 0, win1251_str.c_str(), -1, &wide_str[0], wlen);

    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, NULL, 0, NULL, NULL);
    if (ulen == 0) return win1251_str;

    std::string utf8_str(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], ulen, NULL, NULL);

    // Убираем нулевой терминатор
    utf8_str.pop_back();
    return utf8_str;
}

// Callback функция для записи ответа
static size_t WriteCallback1(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

class DeepseekAPI {
public:
    std::string prompt;
    std::string result;
    
private:
    std::string api_key_;
    std::string base_url_ = "https://openrouter.ai/api/v1/chat/completions";
    //DeepseekAPI(const std::string& api_key = "") : api_key_(api_key) {}

public:
    DeepseekAPI() {
        // Чтобы сайт не забанил, нужно ключ "спрятать"
        api_key_ = "sk-or-v1-795eb396b5b5e8b594748e56bf8fa5";
        api_key_ += "3f5f93a0af01a02df03bab8f5aba7928f8";
    }

    std::string sendPrompt() {
        CURL* curl;
        CURLcode res;
        std::string response;

        curl = curl_easy_init();
        if (curl) {
            // Подготовка JSON данных
            json request_data;
            request_data["model"] = "tngtech/deepseek-r1t2-chimera:free";
            request_data["messages"] = json::array();
            request_data["messages"][0] = {
                {"role", "user"},
                {"content", prompt}
            };
            request_data["stream"] = false;

            std::string json_data = request_data.dump();

            // Настройка заголовков
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            if (!api_key_.empty()) {
                std::string auth_header = "Authorization: Bearer " + api_key_;
                headers = curl_slist_append(headers, auth_header.c_str());
            }

            // НАСТРОЙКИ SSL
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключить проверку сертификата
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Отключить проверку хоста

            // Настройка CURL
            curl_easy_setopt(curl, CURLOPT_URL, base_url_.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback1);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // Выполнение запроса
            res = curl_easy_perform(curl);

            // Очистка
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                return "Error: " + std::string(curl_easy_strerror(res));
            }
        }

        return response;
    }

    std::string parseResponse(const std::string& response) {
        try {
            json json_response = json::parse(response);

            // Проверка на наличие ошибки
            if (json_response.contains("error")) {
                return "API Error: " + json_response["error"]["message"].get<std::string>();
            }

            // Извлечение текста ответа
            if (json_response.contains("choices") && !json_response["choices"].empty()) {
                return json_response["choices"][0]["message"]["content"].get<std::string>();
            }
        }
        catch (const std::exception& e) {
            return "Parse error: " + std::string(e.what());
        }

        return "No response content";
    }
    
    string ИдеальнаяПогода() {
        string result = "18.09; 10 градусов; Погода ясная"
                        "19.09; 15 градусов; Погода ясная"
                        "20.09; 11 градусов; Погода ясная";
        return result;
    }

    string ГрустнаяПогода() {
        string result = "21.09; 10 градусов; Погода ясная"
                        "22.09; 15 градусов; Погода ясная"
                        "23.09; 11 градусов; Средний дождь"
                        "24.09; 11 градусов; Погода ясная"
                        "25.09; 11 градусов; Погода ясная"
                        "26.09; 11 градусов; Погода ясная"
                        "27.09; 11 градусов; Погода ясная";
        return result;
    }
    
    void main() {
        prompt = "Ты автолюбитель. Тебе нравится держать машину в чистоте, но не нравится, что после чистки машины сразу пошёл дождь."
                 "Ты живешь в Ижевске. Тебе необходимо на основании информации о погоде решить стоит ли тебе мыть машину сегодня."
                 "Вот погодные условия на следующие дни. Нужна оценка от 1 до 10."
                 + ГрустнаяПогода();
        prompt = win1251_to_utf8(prompt);
        result = sendPrompt();
        result = parseResponse(result);
    }
};

