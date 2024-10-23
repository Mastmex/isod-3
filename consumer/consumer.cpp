#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>

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

void set_connection(mq_data mq)
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
        // Подтверждение получения сообщения
        channel.ack(deliveryTag);
    });

    // Запускаем цикл событий
    event_base_dispatch(eventBase);

    event_base_free(eventBase);
}

int main() {
    mq_data mq = parse_mq_json("mq-config.json");
    set_connection(mq);    
    return 0;
}
