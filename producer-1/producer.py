import json
import http.client
import pika
import sys

def load_json_file(file_path):
    try:
        with open(file_path, 'r') as file:
            return json.load(file)
    except FileNotFoundError:
        print(f"Ошибка: Файл '{file_path}' не найден.")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Ошибка: Не удалось разобрать JSON из файла '{file_path}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Неизвестная ошибка при загрузке файла '{file_path}': {e}")
        sys.exit(1)

def get_nested_data(data, keys):
    try:
        for key in keys:
            data = data[key]
        return data
    except KeyError as e:
        print(f"Ошибка: Отсутствует ключ {e} в конфигурации.")
        sys.exit(1)
    except TypeError:
        print("Ошибка: Некорректная структура данных при доступе к вложенным ключам.")
        sys.exit(1)
    except Exception as e:
        print(f"Неизвестная ошибка при получении вложенных данных: {e}")
        sys.exit(1)

def make_http_request(host, address, headers):
    try:
        conn = http.client.HTTPSConnection(host)
        conn.request("GET", address, headers=headers)
        res = conn.getresponse()
        if res.status != 200:
            print(f"Ошибка: HTTP-запрос вернул статус {res.status} {res.reason}.")
            sys.exit(1)
        data = res.read()
        conn.close()
        return data
    except http.client.HTTPException as e:
        print(f"Ошибка HTTP-соединения: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Неизвестная ошибка при выполнении HTTP-запроса: {e}")
        sys.exit(1)

def connect_rabbitmq(mq_config):
    try:
        credentials = pika.PlainCredentials(mq_config['login'], mq_config['password'])
        connection_params = pika.ConnectionParameters(
            host=mq_config['hostname'],
            port=mq_config['port'],
            virtual_host=mq_config['virtual_host'],
            credentials=credentials
        )
        connection = pika.BlockingConnection(connection_params)
        channel = connection.channel()
        channel.queue_declare(queue=mq_config['queue_name'], durable=False)
        return connection, channel
    except pika.exceptions.AMQPConnectionError as e:
        print(f"Ошибка подключения к RabbitMQ: {e}")
        sys.exit(1)
    except KeyError as e:
        print(f"Ошибка: Отсутствует ключ {e} в конфигурации RabbitMQ.")
        sys.exit(1)
    except Exception as e:
        print(f"Неизвестная ошибка при подключении к RabbitMQ: {e}")
        sys.exit(1)

def main():
    # Загрузка конфигурации
    config_data = load_json_file('config.json')

    # Присвоение значений переменным
    provider = config_data.get('provider')
    host = config_data.get('host')
    api = config_data.get('api')
    address = config_data.get('address')

    if not all([provider, host, api, address]):
        print("Ошибка: Некоторые обязательные поля отсутствуют в 'config.json'.")
        sys.exit(1)

    path_keys = ['path', 'path1', 'name', 'path2', 'name', 'path3', 'name']
    try:
        path1 = config_data['path']['path1']['name']
        path2 = config_data['path']['path1']['path2']['name']
        path3 = config_data['path']['path1']['path2']['path3']['name']
    except KeyError as e:
        print(f"Ошибка: Отсутствует ключ {e} в пути конфигурации.")
        sys.exit(1)

    # Получение данных по REST
    headers = {
        'x-rapidapi-key': api,
        'x-rapidapi-host': host
    }

    data = make_http_request(host, address, headers)

    # Разбор полученных данных
    try:
        incoming_data = json.loads(data)
    except json.JSONDecodeError as e:
        print(f"Ошибка: Не удалось разобрать JSON из ответа: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Неизвестная ошибка при разборе JSON: {e}")
        sys.exit(1)

    current_price = get_nested_data(incoming_data, [path1, path2, path3])

    # Загрузка конфигурации RabbitMQ
    mq_config = load_json_file('mq-config.json')

    # Подключение к RabbitMQ
    connection, channel = connect_rabbitmq(mq_config)

    # Подготовка и отправка сообщения
    try:
        message_body = json.dumps({"name": provider, "load": str(current_price)})
        channel.basic_publish(
            exchange='',
            routing_key=mq_config['queue_name'],
            body=message_body,
            properties=pika.BasicProperties(
                delivery_mode=1,
            )
        )
        print(f" [x] Отправлено: {message_body}")
    except Exception as e:
        print(f"Ошибка при отправке сообщения в очередь: {e}")
    finally:
        try:
            connection.close()
        except Exception as e:
            print(f"Ошибка при закрытии соединения с RabbitMQ: {e}")

if __name__ == "__main__":
    main()
