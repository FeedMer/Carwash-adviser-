#include "DataBase.h"
#include "WeatherApi.h"
#include "DeepseekAPI.h"
#include <iostream>
using namespace std;

// Просто вызов моих функций
void gd() {
    setlocale(LC_ALL, "ru_RU.UTF-8");
    DeepseekAPI ds;
    DataBase db;
    //ds.main();
    cout << ds.result << endl;

    //db.addMessage("1", ds.prompt, ds.result);
    //auto user = db.getUser("1");
    db.addTelegramUser("test", "test");
    db.outTelegramUsers();
}

int main() {

    //---request example---
    /*MeteoApiConnector api = MeteoApiConnector(56.51, 53.12);
    Weather w = api.getCurrentWeather();
    cout << w << endl;
    return 0;*/

    gd();

    // Не удалять, т.к. закрывается консоль в .exe
    cout << "It works!";
    system("pause");
    return 0;
}
