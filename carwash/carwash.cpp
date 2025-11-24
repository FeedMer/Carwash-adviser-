#include "DataBase.h"
#include "WeatherApi.h"
#include "DeepseekAPI.h"
#include <iostream>

#include "TGBot.h"
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
    //db.addTelegramUser("test", "test");
    //db.outTelegramUsers();
    for (auto user : db.usersForMailing()) {
        cout << user.name << " " << user.telegramId << endl;
    }
}

int main() {
    //---request example---

    // Текущая погода
    
    StormGlassConnector api = StormGlassConnector(56.51, 53.12);
    Weather w = api.getCurrentWeather();
    cout << w << endl;

    ///
    TelegramBot bot("8212512135:AAFFT4JdYLPXnYQrM_EIJ2EF886LBPEqXdI");
    tg.start();
    ///
    cout << "Bot started..." << endl;

    // Погода на 5 дней с максимальными осадками за каждый день
    /*
    StormGlassConnector api = StormGlassConnector(56.51, 53.12);
    vector<Weather> fiveDaysWeather = api.getWeatherForFiveDays();

    cout << "Погода на 5 дней (максимальные осадки за день):\n" << endl;
    for (int i = 0; i < fiveDaysWeather.size(); i++) {
        cout << "День " << (i + 1) << ":\n";
        cout << fiveDaysWeather[i] << endl;
    }
    */

    // gd();

    // Не удалять, т.к. закрывается консоль в .exe
    // cout << "It works!";
    system("pause");
    return 0;
}
