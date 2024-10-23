import json
import mysql.connector
import matplotlib.pyplot as plt
import pandas as pd
import http.client
import base64

def load_db_config(filename):
    """Загрузка конфигурации базы данных из JSON файла."""
    with open(filename, 'r') as f:
        config = json.load(f)
    return config['user'], config['password'], config['container_name']

def load_api_key(filename):
    """Загрузка API ключа из JSON файла."""
    with open(filename, 'r') as f:
        config = json.load(f)
    return config['api']

def fetch_data_from_db(user, password, container_name):
    # Параметры подключения к базе данных
    config = {
        'user': user,
        'password': password,
        'host': 'localhost',  # или '127.0.0.1', если контейнер запущен локально
        'database': 'STOCKS',
    }

    try:
        # Подключение к базе данных
        conn = mysql.connector.connect(**config)
        cursor = conn.cursor()

        # Получение последних 24 записей для каждого значения name
        query1 = """
            SELECT timestamp, value, name 
            FROM data 
            WHERE name = 'Название 1' 
            ORDER BY timestamp DESC 
            LIMIT 24;
        """
        query2 = """
            SELECT timestamp, value, name 
            FROM data 
            WHERE name = 'Название 2' 
            ORDER BY timestamp DESC 
            LIMIT 24;
        """

        # Выполнение запросов
        cursor.execute(query1)
        data1 = cursor.fetchall()

        cursor.execute(query2)
        data2 = cursor.fetchall()

        # Закрытие соединения
        cursor.close()
        conn.close()

        return data1, data2

    except mysql.connector.Error as err:
        print(f"Ошибка: {err}")
        return [], []

def plot_data(data1, data2):
    # Проверка на пустые списки
    if not data1 and not data2:
        print("Входной набор данных пуст.")
        return

    # Преобразование данных в DataFrame
    df1 = pd.DataFrame(data1, columns=['Дата', 'Значение', 'Название'])
    df2 = pd.DataFrame(data2, columns=['Дата', 'Значение', 'Название'])
    
    # Преобразование строки даты в datetime
    df1['Дата'] = pd.to_datetime(df1['Дата'])
    df2['Дата'] = pd.to_datetime(df2['Дата'])
    
    # Установка даты как индекса
    df1.set_index('Дата', inplace=True)
    df2.set_index('Дата', inplace=True)

    # Сортировка по индексу
    df1.sort_index(inplace=True)
    df2.sort_index(inplace=True)

    # Построение графика
    plt.figure(figsize=(10, 6))

    # Первый набор данных
    plt.plot(df1.index, df1['Значение'], marker='o', label=data1[0][2], color='blue')
    
    # Подписи для первого набора данных
    for i in range(len(df1)):
        plt.text(df1.index[i], df1['Значение'].iloc[i], str(df1['Значение'].iloc[i]), 
                 verticalalignment='bottom', horizontalalignment='right', color='blue')

    # Второй набор данных
    plt.plot(df2.index, df2['Значение'], marker='o', label=data2[0][2], color='orange')

    # Подписи для второго набора данных
    for i in range(len(df2)):
        plt.text(df2.index[i], df2['Значение'].iloc[i], str(df2['Значение'].iloc[i]), 
                 verticalalignment='bottom', horizontalalignment='right', color='orange')

    # Форматирование оси X для отображения времени
    plt.xticks(rotation=45)
    plt.xlabel('Время')
    plt.ylabel('Значение')
    plt.title('График значений')
    plt.legend()
    plt.grid()
    
    # Сохранение графика в файл в директории /tmp
    plt.tight_layout()
    plt.savefig('/tmp/plot.png')  # Указываем путь для сохранения
    plt.show()

def upload_image_to_imgbb(image_path, api_key):
    """Загрузка изображения на imgbb."""
    conn = http.client.HTTPSConnection("api.imgbb.com")
    
    # Определение заголовков
    headers = {
        'Content-Type': 'application/json'
    }
    
    # Открытие файла изображения и чтение его в бинарном режиме
    with open(image_path, 'rb') as img_file:
        image_data = img_file.read()

    # Кодирование изображения в base64
    image_base64 = base64.b64encode(image_data).decode('utf-8')

    # Формирование данных для отправки
    payload = {
        'image': image_base64,
        'expiration': 600,
        'key': api_key
    }

    # Преобразование данных в JSON
    json_payload = json.dumps(payload)

    # Отправка POST-запроса
    conn.request("POST", "/1/upload", json_payload, headers)

    # Получение ответа
    res = conn.getresponse()
    data = res.read()

    # Закрытие соединения
    conn.close()

    # Декодирование ответа
    response_json = json.loads(data.decode('utf-8'))
    
    return response_json

# Загрузка конфигурации базы данных
user, password, container_name = load_db_config('db_config.json')

# Загрузка API ключа
api_key = load_api_key('api.json')

# Получение данных из базы данных
data1, data2 = fetch_data_from_db(user, password, container_name)

# Вызов функции для построения графика
plot_data(data1, data2)

# Путь к изображению
image_path = '/tmp/plot.png'

# Вызов функции загрузки изображения
response = upload_image_to_imgbb(image_path, api_key)

# Вывод результата
print(response)
