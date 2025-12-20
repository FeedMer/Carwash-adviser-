#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <json.hpp>

using json = nlohmann::json;

class YandexWeatherParser {
private:
    std::string html_content_;

    // Callback для CURL для записи данных
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t realsize = size * nmemb;
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append(static_cast<char*>(contents), realsize);
        return realsize;
    }

    // Функция для извлечения текста из XML узла
    std::string GetNodeText(xmlNodePtr node) const {
        std::string result;

        for (xmlNodePtr cur = node->children; cur != nullptr; cur = cur->next) {
            if (cur->type == XML_TEXT_NODE && cur->content) {
                result += reinterpret_cast<const char*>(cur->content);
            }
        }

        // Удаляем лишние пробелы и переносы
        result.erase(0, result.find_first_not_of(" \n\r\t"));
        result.erase(result.find_last_not_of(" \n\r\t") + 1);

        return result;
    }

    // Функция для поиска элемента по атрибуту class
    xmlNodePtr FindElementByClass(xmlNodePtr node, const std::string& className) const {
        xmlNodePtr cur_node = nullptr;

        for (cur_node = node; cur_node != nullptr; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                xmlChar* prop = xmlGetProp(cur_node, (const xmlChar*)"class");
                if (prop != nullptr) {
                    std::string classValue((const char*)prop);
                    xmlFree(prop);

                    if (classValue.find(className) != std::string::npos) {
                        return cur_node;
                    }
                }

                // Рекурсивно ищем в дочерних элементах
                xmlNodePtr child_result = FindElementByClass(cur_node->children, className);
                if (child_result != nullptr) {
                    return child_result;
                }
            }
        }

        return nullptr;
    }

public:
    YandexWeatherParser() {
        // Инициализируем библиотеки
        curl_global_init(CURL_GLOBAL_DEFAULT);
        xmlInitParser();
    }

    ~YandexWeatherParser() {
        curl_global_cleanup();
        xmlCleanupParser();
    }

    // Загружаем HTML страницу
    bool LoadPage(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize CURL" << std::endl;
            return false;
        }

        std::string buffer;

        // НАСТРОЙКИ SSL
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Отключить проверку сертификата
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Отключить проверку хоста

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_cleanup(curl);
        html_content_ = buffer;
        return !html_content_.empty();
    }

    // Парсим погодные данные
    std::string ParseWeatherData() {
        if (html_content_.empty()) {
            return "No HTML content loaded";
        }

        // Парсим HTML
        htmlDocPtr doc = htmlReadDoc(
            (const xmlChar*)html_content_.c_str(),
            nullptr,
            nullptr,
            HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
        );

        if (!doc) {
            return "Failed to parse HTML";
        }

        std::string result;

        // Ищем главный контейнер с прогнозом
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (!root) {
            xmlFreeDoc(doc);
            return "No root element found";
        }

        // Создаем контекст XPath
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context) {
            xmlFreeDoc(doc);
            return "Failed to create XPath context";
        }

        // Регистрируем namespace
        xmlXPathRegisterNs(context, (const xmlChar*)"h", (const xmlChar*)"http://www.w3.org/1999/xhtml");

        try {
            // Ищем блок с прогнозом по классу
            xmlXPathObjectPtr xpathResult = xmlXPathEvalExpression(
                (const xmlChar*)"//ul[@class='MainPage_appForecast__5mP3d']",
                context
            );

            if (!xpathResult || !xpathResult->nodesetval || xpathResult->nodesetval->nodeNr == 0) {
                // Попробуем другой способ поиска
                xmlNodePtr forecastContainer = FindElementByClass(root, "MainPage_appForecast__5mP3d");

                if (!forecastContainer) {
                    xmlXPathFreeObject(xpathResult);
                    xmlXPathFreeContext(context);
                    xmlFreeDoc(doc);
                    return "Weather forecast container not found";
                }

                // Парсим каждый день
                result = ParseForecastDays(forecastContainer);
            }
            else {
                // Нашли через XPath
                xmlNodeSetPtr nodes = xpathResult->nodesetval;
                for (int i = 0; i < nodes->nodeNr; i++) {
                    result += ParseForecastDays(nodes->nodeTab[i]);
                }
                xmlXPathFreeObject(xpathResult);
            }

        }
        catch (const std::exception& e) {
            result = "Error parsing weather data: ";
            result += e.what();
        }

        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);

        return result;
    }

private:
    // Парсим данные по дням
    std::string ParseForecastDays(xmlNodePtr container) const {
        std::string result;

        // Ищем все элементы li с классом AppForecastDay_dayCard__rIRAn
        xmlNodePtr cur_node = nullptr;
        int dayCount = 0;

        for (cur_node = container->children; cur_node != nullptr; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                xmlChar* prop = xmlGetProp(cur_node, (const xmlChar*)"class");
                if (prop != nullptr) {
                    std::string classValue((const char*)prop);
                    xmlFree(prop);

                    if (classValue.find("AppForecastDay_dayCard__rIRAn") != std::string::npos) {
                        dayCount++;
                        result += "Day " + std::to_string(dayCount) + ":\n";
                        result += ParseDayForecast(cur_node);
                        result += "\n" + std::string(50, '-') + "\n";
                    }
                }
            }
        }

        if (dayCount == 0) {
            result = "No forecast days found\n";
        }

        return result;
    }

    // Парсим данные одного дня
    std::string ParseDayForecast(xmlNodePtr dayNode) const {
        std::string result;

        // Ищем заголовок дня
        xmlNodePtr titleNode = FindElementByClass(dayNode, "AppForecastDayHeader_dayTitle__23ecF");
        if (titleNode) {
            std::string dayTitle = GetNodeText(titleNode);
            result += "Date: " + dayTitle + "\n";
        }

        // Парсим времена суток
        result += ParseDayParts(dayNode);

        // Парсим дополнительную информацию
        result += ParseDayAdditionalInfo(dayNode);

        return result;
    }

    // Парсим данные по временам суток (утро, день, вечер, ночь)
    std::string ParseDayParts(xmlNodePtr dayNode) const {
        std::string result = "Forecast by time of day:\n";

        // Массив названий времени суток
        const std::vector<std::string> timeOfDay = { "Утром", "Днём", "Вечером", "Ночью" };
        const std::vector<std::string> timePrefixes = { "m", "d", "e", "n" };

        for (size_t i = 0; i < timeOfDay.size(); i++) {
            std::string timeStr = timeOfDay[i];
            std::string prefix = timePrefixes[i];

            // Температура
            xmlNodePtr tempNode = FindElementByClass(dayNode, "AppForecastDayPart_temp__kKbJG");
            if (tempNode) {
                std::string temp = GetNodeText(tempNode);
                result += timeStr + " temperature: " + temp + "\n";
            }

            // Ощущается как
            xmlNodePtr feelsNode = FindElementByClass(dayNode, "AppForecastDayPart_showNarrow__fTNAB");
            if (feelsNode) {
                std::string feels = GetNodeText(feelsNode);
                result += timeStr + " feels like: " + feels + "\n";
            }

            // Погодные условия
            xmlNodePtr conditionNode = FindElementByClass(dayNode, "AppForecastDayPart_text__dFFbf");
            if (conditionNode) {
                std::string condition = GetNodeText(conditionNode);
                result += timeStr + " condition: " + condition + "\n";
            }

            // Ветер
            xmlNodePtr windNode = FindElementByClass(dayNode, "AppForecastDayPart_wind_formatted__mBWvC");
            if (windNode) {
                std::string wind = GetNodeText(windNode);
                result += timeStr + " wind: " + wind + "\n";
            }

            // Направление ветра
            xmlNodePtr windDirNode = FindElementByClass(dayNode, "AppForecastDayPart_direction__value__kCeBF");
            if (windDirNode) {
                std::string windDir = GetNodeText(windDirNode);
                result += timeStr + " wind direction: " + windDir + "\n";
            }

            // Влажность
            xmlNodePtr humidityNode = nullptr;
            xmlNodePtr cur = dayNode->children;
            int humidityCount = 0;

            while (cur != nullptr) {
                if (cur->type == XML_ELEMENT_NODE) {
                    xmlChar* prop = xmlGetProp(cur, (const xmlChar*)"class");
                    if (prop != nullptr) {
                        std::string classValue((const char*)prop);
                        xmlFree(prop);

                        if (classValue.find("AppForecastDayPart_showNarrow__fTNAB") != std::string::npos) {
                            humidityCount++;
                            if (humidityCount == i + 1) {
                                std::string humidity = GetNodeText(cur);
                                result += timeStr + " humidity: " + humidity + "\n";
                                break;
                            }
                        }
                    }
                }
                cur = cur->next;
            }

            // Давление
            xmlNodePtr pressureNode = nullptr;
            cur = dayNode->children;
            int pressureCount = 0;

            while (cur != nullptr) {
                if (cur->type == XML_ELEMENT_NODE) {
                    xmlChar* prop = xmlGetProp(cur, (const xmlChar*)"class");
                    if (prop != nullptr) {
                        std::string classValue((const char*)prop);
                        xmlFree(prop);

                        if (classValue.find("AppForecastDayPart_showNarrow__fTNAB") != std::string::npos) {
                            pressureCount++;
                            if (pressureCount == i + 1) {
                                std::string pressure = GetNodeText(cur);
                                result += timeStr + " pressure: " + pressure + " mm Hg\n";
                                break;
                            }
                        }
                    }
                }
                cur = cur->next;
            }

            result += "\n";
        }

        return result;
    }

    // Парсим дополнительную информацию о дне
    std::string ParseDayAdditionalInfo(xmlNodePtr dayNode) const {
        std::string result = "Additional information:\n";

        // Световой день
        xmlNodePtr daylightNode = FindElementByClass(dayNode, "AppForecastDayDuration_text__5jxpv");
        if (daylightNode) {
            std::string daylight = GetNodeText(daylightNode);
            result += "Daylight: " + daylight + "\n";
        }

        // Температура воды
        xmlNodePtr waterTempNode = FindElementByClass(dayNode, "AppForecastDayDuration_water__QQo5T");
        if (waterTempNode) {
            // Ищем температуру воды в дочерних элементах
            xmlNodePtr cur = waterTempNode->children;
            while (cur != nullptr) {
                if (cur->type == XML_ELEMENT_NODE &&
                    xmlStrcmp(cur->name, (const xmlChar*)"div") == 0) {
                    std::string waterTemp = GetNodeText(cur);
                    result += "Water temperature: " + waterTemp + "\n";
                    break;
                }
                cur = cur->next;
            }
        }

        // Фаза луны
        xmlNodePtr moonNode = FindElementByClass(dayNode, "AppForecastDayDuration_moon__ZS84W");
        if (moonNode) {
            xmlNodePtr cur = moonNode->children;
            while (cur != nullptr) {
                if (cur->type == XML_ELEMENT_NODE &&
                    xmlStrcmp(cur->name, (const xmlChar*)"div") == 0) {
                    std::string moonPhase = GetNodeText(cur);
                    result += "Moon phase: " + moonPhase + "\n";
                    break;
                }
                cur = cur->next;
            }
        }

        // УФ-индекс
        xmlNodePtr uvNode = FindElementByClass(dayNode, "AppForecastDayDuration_text_overflow__4esZL");
        if (uvNode) {
            std::string uvIndex = GetNodeText(uvNode);
            result += "UV index: " + uvIndex + "\n";
        }

        // Магнитное поле
        xmlNodePtr magneticNode = FindElementByClass(dayNode, "AppForecastDayDuration_value__SOslN");
        if (magneticNode) {
            std::string magnetic = GetNodeText(magneticNode);
            result += "Magnetic field: " + magnetic + "\n";
        }

        // Общая влажность и давление дня
        xmlNodePtr summaryNode = FindElementByClass(dayNode, "AppForecastDaySummary_content__7MT7D");
        if (summaryNode) {
            xmlNodePtr cur = summaryNode->children;
            int summaryCount = 0;

            while (cur != nullptr) {
                if (cur->type == XML_ELEMENT_NODE) {
                    xmlChar* prop = xmlGetProp(cur, (const xmlChar*)"class");
                    if (prop != nullptr) {
                        std::string classValue((const char*)prop);
                        xmlFree(prop);

                        if (classValue.find("AppForecastDaySummary_content_item___3CqH") != std::string::npos) {
                            summaryCount++;
                            std::string summaryItem = GetNodeText(cur);

                            if (summaryCount == 1) {
                                result += "Average humidity: " + summaryItem + "\n";
                            }
                            else if (summaryCount == 2) {
                                result += "Average pressure: " + summaryItem + "\n";
                            }
                        }
                    }
                }
                cur = cur->next;
            }
        }

        return result;
    }
};

