#include <iostream>
#include <mysqlx/xdevapi.h>
using namespace std;

mysqlx::Session sqlConnection() {
    //mysqlx::Session session("mysqlx://remote_root:123@MySQL-8.4/carwash");
    mysqlx::Session session("mysqlx://remote_root:123@26.74.255.166:33060/carwash");
    cout << "Connected to MySQL successfully!" << endl;
    return session;
}

bool addTelegramUser(string telegramId, string name) {
    auto session = sqlConnection();
    auto query = session.sql("INSERT INTO telegram_users(telegram_id, name) VALUES (?, ?)");
    query.bind(telegramId, name);
    query.execute();
    return true;
}

void outTelegramUsers() {
    auto session = sqlConnection();
    auto result = session.sql("SELECT * from telegram_users").execute();
    while (auto row = result.fetchOne()) {
        cout << row[0] << ' ' << row[1] << endl;
    }
}

int main(){
    try {
        //addTelegramUser("kek", "chebureck");
        outTelegramUsers();
    } catch (const mysqlx::Error& err) {
        std::cerr << "SQL Error: " << err.what() << std::endl;
    }
    system("pause");
}

