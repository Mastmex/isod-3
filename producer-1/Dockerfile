FROM debian:bookworm-20241016

# Установка Python и cron
RUN apt-get update && \
    apt-get install -y python3 cron python3-pip python3-venv && \
    apt-get clean

# Копируем необходимые файлы
COPY producer.py /home/prod/
COPY config.json /home/prod/
COPY mq-config.json /home/prod/

# Создаем виртуальное окружение и устанавливаем pika
RUN python3 -m venv /home/prod/myenv && \
    /home/prod/myenv/bin/pip install pika

# Копируем файл cron в контейнер
COPY producer-cron /etc/cron.d/producer-cron

# Устанавливаем права доступа и добавляем cron задачу
RUN chmod 0644 /etc/cron.d/producer-cron && \
    crontab /etc/cron.d/producer-cron

# Создаем файл для логов cron
RUN touch /var/log/cron.log

# Запускаем cron в фоновом режиме и выводим логи
CMD cron && tail -f /var/log/cron.log
