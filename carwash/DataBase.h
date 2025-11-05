#pragma once
#include <mysqlx/xdevapi.h>
#include <vector>
#include <iostream>
using namespace std;

struct TelegramUser {
public:
    string telegramId;
    string name;
    int status = 0;
};

class DataBase{
public:
    DataBase() {

    }

    // Создание подключения к базе данных
    mysqlx::Session sqlConnection() {
        mysqlx::Session session("mysqlx://remote_root:123@26.74.255.166:33060/carwash");
        cout << "Connected to MySQL successfully!" << endl;
        return session;
    }

    // Добавление телеграм пользователя
    bool addTelegramUser(string telegramId, string name) {
        auto session = sqlConnection();
        auto query = session.sql("INSERT INTO telegram_users(telegram_id, name) VALUES (?, ?)");
        query.bind(telegramId, name);
        query.execute();
        return setUserStatus(telegramId, 1);
    }

    // Установка статуса пользователя (1 - активный, 0 - неактивный)
    bool setUserStatus(string telegramId, int status) {
        auto session = sqlConnection();
        auto query = session.sql("INSERT INTO users_status(telegram_id, status) VALUES (?, ?)");
        query.bind(telegramId, status);
        query.execute();
        return true;
    }

    // Добавление информации об отправленном сообщении
    bool addMessage(string telegramId, string prompt, string result) {
        auto session = sqlConnection();
        auto query = session.sql("INSERT INTO sending_messages(telegram_id, prompt, result) VALUES (?, ?, ?)");
        query.bind(telegramId, prompt, result);
        query.execute();
        return true;
    }

    // Посмотреть список пользователей
    void outTelegramUsers() {
        string queryText = 
            R"(SELECT 
                    telegram_users.telegram_id,
                    telegram_users.name,
                    users_status.status,
                    DATE_FORMAT(users_status.date, '%d.%m.%y %h:%i')
                FROM 
                    telegram_users
                    LEFT JOIN users_status
                        ON telegram_users.telegram_id = users_status.telegram_id)";

        auto session = sqlConnection();
        auto result = session.sql(queryText).execute();
        while (auto row = result.fetchOne()) {
            cout << row[0] << ' ' << row[1] << ' ' << row[2] << ' ' << row[3] << endl;
        }
    }

    // Получить всю информацию о пользователе
    TelegramUser getUser(string telegramId) {
        TelegramUser user;
        string queryText =
            R"(SELECT
                    telegram_users.telegram_id,
                    telegram_users.name,
                    users_status.status,
                    DATE_FORMAT(users_status.date, '%d.%m.%y %h:%i')
                FROM 
                    telegram_users
                    LEFT JOIN users_status
                        ON telegram_users.telegram_id = users_status.telegram_id
                WHERE
                    telegram_users.telegram_id = ?
                ORDER BY 
                    users_status.date DESC
                LIMIT 1)";

        try {
            auto session = sqlConnection();
            auto query = session.sql(queryText);
            query.bind(telegramId);
            mysqlx::SqlResult result = query.execute();

            if (auto row = result.fetchOne()) {
                user.telegramId = string(row[0]);
                user.name = string(row[1]);
                if (!row[2].isNull()) {
                    user.status = row[2];
                }
            }
        }
        catch (const mysqlx::Error& e) {
            cerr << "MySQL error: " << e.what() << endl;
        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
        }


        return user;
    }

    // Получение пользователей, которым необходимо выполнить рассылку сообщений
    vector <TelegramUser> usersForMailing() {
        vector <TelegramUser> users;
        string queryText =
            R"(SELECT 
                    telegram_id,
                    name,
                    status,
                    formatted_date
                FROM (
                    SELECT
                        tu.telegram_id,
                        tu.name,
                        us.status,
                        DATE_FORMAT(us.date, '%d.%m.%y %H:%i') AS formatted_date,
                        ROW_NUMBER() OVER (
                            PARTITION BY tu.telegram_id 
                            ORDER BY us.date DESC
                        ) AS rn
                    FROM 
                        telegram_users tu
                        LEFT JOIN users_status us
                            ON tu.telegram_id = us.telegram_id
                ) AS ranked
                WHERE rn = 1 AND status = 1)";

        auto session = sqlConnection();
        auto query = session.sql(queryText);
        mysqlx::SqlResult result = query.execute();

        while (auto row = result.fetchOne()) {
            TelegramUser user;
            user.telegramId = string(row[0]);
            user.name = string(row[1]);
            user.status = 1;
            users.push_back(user);
        }

        return users;
    }

    void sqlExamples() {
        try {
            //addTelegramUser("kek", "chebureck");
            outTelegramUsers();
        }
        catch (const mysqlx::Error& err) {
            std::cerr << "SQL Error: " << err.what() << std::endl;
        }
    }
};


