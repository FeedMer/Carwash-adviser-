#include "DataBase.h"
#include "WeatherApi.h"
#include "DeepseekAPI.h"
#include <iostream>
using namespace std;

int main(){
    
    //DataBase db;
    //db.sqlExamples();

    //---request example---
    /*MeteoApiConnector api = MeteoApiConnector(56.51, 53.12);
    Weather w = api.getCurrentWeather();
    cout << w << endl;
    return 0;*/

    setlocale(LC_ALL, "ru_RU.UTF-8");
    DeepseekAPI ds;
    ds.main();
    cout << ds.result << endl;
}
