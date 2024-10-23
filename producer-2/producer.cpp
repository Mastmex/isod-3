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

struct api_data
{
    string provider;
    string host;
    string api;
    string address;
    string path1, path2, path3;
};

struct mq_data
{
    string hostname;
    string port;
    string user;
    string password;
    string vhost;
    string queue;
};


size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    // Добавляем полученные данные в строку
    static_cast<std::string *>(userp)->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

double get_price(api_data api)
{
    std::string responseBuffer;
    CURL *hnd = curl_easy_init();
    if (hnd)
    {
        // Установка параметров запроса
        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(hnd, CURLOPT_URL, ("https://" + api.host + api.address).c_str()); // Укажите URL

        // Установка заголовков
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("x-rapidapi-key: " + api.api).c_str());
        headers = curl_slist_append(headers, ("x-rapidapi-host: " + api.host).c_str());
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

        // Установка функции обратного вызова для записи ответа
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &responseBuffer);

        // Выполнение запроса
        CURLcode ret = curl_easy_perform(hnd);
        if (ret != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(ret) << std::endl;
        }
        else
        {
            // Вывод полученного JSON
            std::cout << "Response: " << responseBuffer << std::endl;
        }

        // Освобождение ресурсов
        curl_slist_free_all(headers);
        curl_easy_cleanup(hnd);
        
    }
    // Получение данных о цене TSLA
    json json_data = json::parse(responseBuffer);
    return json_data[api.path1][stoi(api.path2)][api.path3];
}

api_data parse_api_json(string filename)
{
    ifstream file(filename);
    json data;
    file >> data;
    file.close();
    return {data["provider"],
            data["host"],
            data["api"],
            data["address"],
            data["path"]["path1"]["name"],
            data["path"]["path1"]["path2"]["name"],
            data["path"]["path1"]["path2"]["path3"]["name"]};
}

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

void send_message(mq_data mq, double load, string name)
{
    struct event_base *eventBase = event_base_new();
    AMQP::LibEventHandler handler(eventBase);

    // Подключаемся к RabbitMQ
    AMQP::TcpConnection connection(&handler, AMQP::Address(mq.hostname, stoi(mq.port), AMQP::Login(mq.user, mq.password), mq.vhost));
    AMQP::TcpChannel channel(&connection);
     channel.declareQueue(mq.queue).onSuccess([]() {
        std::cout << "Queue declared successfully!" << std::endl;
    });
    json message = {{"name",name},{"load",to_string(load)}};
    channel.publish("", mq.queue, message.dump());
    std::cout << "Message sent: " << message.dump() << std::endl;
    connection.close();

    // Запускаем цикл событий
    event_base_dispatch(eventBase);

    event_base_free(eventBase);
}

int main()
{
    api_data api = parse_api_json("config.json");
    double price = get_price(api);
    mq_data mq = parse_mq_json("mq-config.json");
    send_message(mq,price,api.provider);
    return 0;
}
