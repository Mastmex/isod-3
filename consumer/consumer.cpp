#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <strstream>
#include <ctime>

using json = nlohmann::json;
using namespace std;

struct mq_data
{
    string hostname;
    string port;
    string user;
    string password;
    string vhost;
    string queue;
};

struct db_data
{
    string user;
    string password;
    string container_name;
};

mq_data parse_mq_json(string filename)
{
    ifstream file(filename);
    json data;
    file >> data;
    file.close();
    return {data["hostname"],
            data["port"],
            data["login"],
            data["password"],
            data["virtual_host"],
            data["queue_name"]};
}

db_data parse_db_json(string filename)
{
    ifstream file(filename);
    json data;
    file >> data;
    file.close();
    return {data["user"],
            data["password"],
            data["container_name"]};
}

string get_formatted_time()
{
    time_t now = time(nullptr);
    tm* localtime = std::localtime(&now);
    std::ostringstream oss;
    oss<< std::put_time(localtime,"%Y-%m-%d %H:%M:%S");
    return oss.str();
}
void insert_in_db(json load,db_data db)
{
     try {
        // Получаем драйвер MySQL
        sql::mysql::MySQL_Driver *driver;
        sql::Connection *con;
        driver = sql::mysql::get_mysql_driver_instance();

        // Подключаемся к MySQL без указания базы данных, чтобы создать ее
        con = driver->connect(db.container_name, db.user, db.password);

        // Создание базы данных, если не существует
        unique_ptr<sql::Statement> stmt(con->createStatement());
        stmt->execute("CREATE DATABASE IF NOT EXISTS STOCKS");

        // Подключаемся к базе данных
        con->setSchema("STOCKS");

        // Создание таблицы, если не существует
        stmt->execute("CREATE TABLE IF NOT EXISTS data ("
                      "id INT AUTO_INCREMENT PRIMARY KEY,"
                      "timestamp DATETIME,"
                      "value DOUBLE,"
                      "name VARCHAR(50))"); // Добавлен столбец name

        // Подготовить запрос для вставки данных
        sql::PreparedStatement *pstmt = con->prepareStatement("INSERT INTO data (timestamp, value, name) VALUES (?, ?, ?)");

        string value = load["load"];
        string name = load["name"];
        // Получаем текущее время
        double doubleValue = stod(value); // Пример значения
        string nameValue = name; // Пример значения для name

        // Установка параметров
        pstmt->setString(1, get_formatted_time()); // Конвертация времени в строку
        pstmt->setDouble(2, doubleValue);
        pstmt->setString(3, nameValue); // Установка значения для name

        // Выполнить запрос
        pstmt->execute();

        // Освобождение ресурсов
        delete pstmt;
        delete con;

        std::cout << "Данные успешно отправлены!" << std::endl;
    } catch (sql::SQLException &e) {
        std::cerr << "Ошибка SQL: " << e.what() << std::endl;
    }

}

void set_connection(mq_data mq,db_data db)
{
    struct event_base *eventBase = event_base_new();
    AMQP::LibEventHandler handler(eventBase);

    AMQP::TcpConnection connection(&handler, AMQP::Address(mq.hostname, stoi(mq.port), AMQP::Login(mq.user, mq.password), mq.vhost));
    AMQP::TcpChannel channel(&connection);

    // Декларация очереди для получения сообщений
    const std::string requestQueue = mq.queue;
    channel.declareQueue(requestQueue).onSuccess([]() {
        std::cout << "Request queue declared!" << std::endl;
    });

    // Обработка полученного сообщения
    channel.consume(requestQueue).onReceived([&](const AMQP::Message &msg, uint64_t deliveryTag, bool redelivered) {
        std::cout << "Received message: " << msg.body() << std::endl;
        string got_load(msg.body());
        json data = json::parse(got_load.substr(0, got_load.find("}"))+"}");
        insert_in_db(data,db);
        channel.ack(deliveryTag);
    });

    // Запускаем цикл событий
    event_base_dispatch(eventBase);

    event_base_free(eventBase);
}

int main() {
    mq_data mq = parse_mq_json("mq-config.json");
    db_data db = parse_db_json("mysql-config.json");
    set_connection(mq,db);
    return 0;
}
