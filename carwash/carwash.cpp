#include "DataBase.h"
#include "WeatherApi.h"
using namespace std;

int main(){
    
    DataBase db;
    db.sqlExamples();

    //---request example---
    // 
    //MeteoApiConnector api = MeteoApiConnector(56.51, 53.12);
    //string request = api.makeRequest();
    //cout << request << endl;
    system("pause");
    return 0;
}
