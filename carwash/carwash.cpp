#include "DataBase.h"
#include "WeatherApi.h"
#include <iostream>
using namespace std;

int main(){
    
    //DataBase db;
    //db.sqlExamples();

    //---request example---
    MeteoApiConnector api = MeteoApiConnector(56.51, 53.12);
    Weather w = api.getCurrentWeather();
    cout << w << endl;
    return 0;
}
