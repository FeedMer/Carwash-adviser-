#pragma once
#include <mysqlx/xdevapi.h>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

struct TelegramUser {
public:
    string telegramId;
    string name;
    int status = 0;
};

class DataBase{
public:
    int mailingFrequency = 0;

    DataBase() {
        ifstream filekey("mailingFrequency.txt");
        filekey >> mailingFrequency;
        filekey.close();
        cout << "mailingFrequency=" << mailingFrequency << endl;
    }

    // Создание подключения к базе данных
    mysqlx::Session sqlConnection() {
        mysqlx::Session session("mysqlx://remote_root:123@MySQL-8.4:33060/carwash");
        cout << "Connected to MySQL successfully!" << endl;
        return session;
    }

    // Добавление телеграм пользователя
    bool addTelegramUser(string telegramId, string name) {
        try {
            auto session = sqlConnection();
            auto query = session.sql("INSERT INTO telegram_users(telegram_id, name) VALUES (?, ?)");
            query.bind(telegramId, name);
            query.execute();
            return setUserStatus(telegramId, 1);
        }
        catch (const mysqlx::Error& err) {
            cout << "Error: " << err.what() << std::endl;
        }
        return false;
    }

    // Установка статуса пользователя (1 - активный, 0 - неактивный)
    bool setUserStatus(string telegramId, int status) {
        try {
            auto session = sqlConnection();
            auto query = session.sql("INSERT INTO users_status(telegram_id, status) VALUES (?, ?)");
            query.bind(telegramId, status);
            query.execute();
            return true;
        }
        catch (const mysqlx::Error& err) {
            cout << "Error: " << err.what() << std::endl;
        }
        return false;
    }

    // Добавление информации об отправленном сообщении
    bool addMessage(string telegramId, string prompt, string result) {
        try {
            auto session = sqlConnection();
            auto query = session.sql("INSERT INTO sending_messages(telegram_id, prompt, result) VALUES (?, ?, ?)");
            query.bind(telegramId, prompt, result);
            query.execute();
            return true;
        }
        catch (const mysqlx::Error& err) {
            cout << "Error: " << err.what() << std::endl;
        }
        return false;
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

        try {
            auto session = sqlConnection();
            auto result = session.sql(queryText).execute();
            while (auto row = result.fetchOne()) {
                cout << row[0] << ' ' << row[1] << ' ' << row[2] << ' ' << row[3] << endl;
            }
        }
        catch (const mysqlx::Error& err) {
            cout << "Error: " << err.what() << std::endl;
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
                    ranked.telegram_id,
                    ranked.name,
                    ranked.status,
                    ranked.formatted_date,
                    sm.last_message_date,
                    DATE_FORMAT(sm.last_message_date, '%d.%m.%y %H:%i') as last_message_formatted
                FROM (
                    SELECT
                        telegram_users.telegram_id,
                        telegram_users.name,
                        users_status.status,
                        DATE_FORMAT(users_status.date, '%d.%m.%y %H:%i') AS formatted_date,
                        ROW_NUMBER() OVER (
                            PARTITION BY telegram_users.telegram_id 
                            ORDER BY users_status.date DESC
                        ) AS _row_number
                    FROM 
                        telegram_users AS telegram_users
                        LEFT JOIN users_status AS users_status
                            ON telegram_users.telegram_id = users_status.telegram_id
                ) AS ranked
                LEFT JOIN (
                    SELECT 
                        telegram_id,
                        MAX(date) as last_message_date
                    FROM sending_messages
                    GROUP BY telegram_id
                ) AS sm ON ranked.telegram_id = sm.telegram_id
                WHERE ranked._row_number = 1 
                    AND ranked.status = 1
                    AND (sm.last_message_date IS NULL OR sm.last_message_date < NOW() - INTERVAL ? HOUR))";

        try {
            auto session = sqlConnection();
            auto query = session.sql(queryText);
            query.bind(mailingFrequency);
            mysqlx::SqlResult result = query.execute();

            while (auto row = result.fetchOne()) {
                TelegramUser user;
                user.telegramId = string(row[0]);
                user.name = string(row[1]);
                user.status = 1;
                users.push_back(user);
            }
        }
        catch (const mysqlx::Error& e) {
            cerr << "MySQL error: " << e.what() << endl;
        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
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


