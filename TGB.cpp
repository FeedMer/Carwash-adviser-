#include <tgbot/tgbot.h>
#include <iostream>
#include <thread>
#include <DataBase.h>

std::pair<std::string, std::string> getOptimalWashDay(std::string city) {
    //...
    return { "Пятница", "Ожидается сухая погода, минимальный риск загрязнения" };
}

//уведомление пользователя
void sendProactiveNotification(const TgBot::Bot& bot, const TelegramUser& user) {
    auto [day, reason] = getOptimalWashDay(user.city);
    std::string msg = "Оптимальный день для мойки машины в городе " + user.city + ": *" + day + "*.\nПричина: " + reason;
    bot.getApi().sendMessage(user.telegramId, msg, false, 0, nullptr, "Markdown");
}

//меню
TgBot::ReplyKeyboardMarkup::Ptr getMainMenu() {
    auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    keyboard->oneTimeKeyboard = false;
    keyboard->resizeKeyboard = true;
    keyboard->keyboard = {
        {TgBot::KeyboardButton::Ptr(new TgBot::KeyboardButton{"Стоит ли мыть сегодня машину?"})},
        {TgBot::KeyboardButton::Ptr(new TgBot::KeyboardButton{"Я помыл машину"})},
        {TgBot::KeyboardButton::Ptr(new TgBot::KeyboardButton{"Отписаться от уведомлений"})},
        {TgBot::KeyboardButton::Ptr(new TgBot::KeyboardButton{"Настройки"})}
    };
    return keyboard;
}

int main() {
    TgBot::Bot bot("8212512135:AAFFT4JdYLPXnYQrM_EIJ2EF886LBPEqXdI");
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        std::string userId = std::to_string(message->from->id);
        std::string name = message->from->firstName;
        addTelegramUser(userId, name);
        std::string text = "Привет. Я помогу определить лучший день для мойки машины!";
        TgBot::ReplyKeyboardMarkup::Ptr kb(new TgBot::ReplyKeyboardMarkup);
        kb->resizeKeyboard = true;
        kb->keyboard = { {TgBot::KeyboardButton::Ptr(new TgBot::KeyboardButton{"Отправить геолокацию"})} };
        kb->keyboard[0][0]->requestLocation = true;
        bot.getApi().sendMessage(message->chat->id, text, false, 0, kb);
        });

    //голокация
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (message->location) {
            double lat = message->location->latitude;
            double lon = message->location->longitude;
            std::string userId = std::to_string(message->from->id);
            std::string city = "Ижевск";
            setUserStatus(userId, 1);
            bot.getApi().sendMessage(message->chat->id,
                "Ваше местоположение сохранено как: " + city,
                false, 0, getMainMenu());
        }
        else if (message->text == "Стоит ли мыть сегодня?") {
            auto user = getUser(std::to_string(message->from->id));
            auto [day, reason] = getOptimalWashDay(user.city);
            bot.getApi().sendMessage(message->chat->id, response);
            addMessage(user.telegramId, message->text, response);
        }
        else if (message->text == "Я помыл машину") {
            bot.getApi().sendMessage(message->chat->id, "Отлично! Цикл рекомендаций начат заново.");
        }
        else if (message->text == "Отписаться от уведомлений") {
            setUserStatus(std::to_string(message->from->id), 0);
            bot.getApi().sendMessage(message->chat->id, "Уведомления временно отключены.");
        }
        else if (message->text == "Настройки") {
            bot.getApi().sendMessage(message->chat->id, "Введите новый город или отправьте геолокацию");
        }
        else if (!message->text.empty() && message->text != "/start") {
            std::string city = message->text;
            bot.getApi().sendMessage(message->chat->id,
                "Город установлен: " + city,
                false, 0, getMainMenu());
        }
        });
    std::thread notifier([&bot]() {
        while (true) {
            TelegramUser demoUser{ "123456789", "Demo", "Москва", 1 };
            sendProactiveNotification(bot, demoUser);
            std::this_thread::sleep_for(std::chrono::hours(24));
        }
        });
    notifier.detach();
    TgBot::TgLongPoll longPoll(bot);
    std::cout << "Бот запущен" << std::endl;
    while (true) {
        try {
            longPoll.start();
        }
        catch (std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
    return 0;
}
