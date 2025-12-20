#include "DataBase.h"
#include "WeatherApi.h"
#include "DeepseekAPI.h"
#include <iostream>
#include "TGBot.h"

using namespace std;

// Просто вызов моих функций
void gd() {
    DeepseekAPI ds;
    DataBase db;
    
    ds.main();
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
    setlocale(LC_ALL, "Russian");
    cout << "Запуск бота..." << endl;
 
    //---request example---
    //MeteoblueConnector mbc = MeteoblueConnector(56.84, 53.20);    // ширина и долгота (для ижевска: 56.84, 53.20)
    //vector<Weather> ws = mbc.getWeathers(5);                      // аргумент колво дней для запроса
    //for (int i = 0; i < 5; i++) {
    //    cout << ws[i] << endl;
    //}

    //SetConsoleOutputCP(CP_UTF8);
    //SetConsoleCP(CP_UTF8);
    //std::cout << "Bot started..." << std::endl;

    TelegramBot bot("8212512135:AAFFT4JdYLPXnYQrM_EIJ2EF886LBPEqXdI");
    bot.start();

    //gd();

    // Не удалять, т.к. закрывается консоль в .exe
    // cout << "It works!";
    system("pause");
    return 0;
}
