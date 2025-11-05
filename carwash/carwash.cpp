#include "DataBase.h"
#include "WeatherApi.h"
#include "DeepseekAPI.h"
#include <iostream>
using namespace std;

// Просто вызов моих функций
void gd() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    DeepseekAPI ds;
    ds.main();
    cout << ds.result << endl;

    DataBase db;
    //db.sqlExamples();
    db.addMessage("1", ds.prompt, ds.result);
}

int main() {

    //---request example---
    /*MeteoApiConnector api = MeteoApiConnector(56.51, 53.12);
    Weather w = api.getCurrentWeather();
    cout << w << endl;
    return 0;*/

    gd();
}
