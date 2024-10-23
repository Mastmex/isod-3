FROM debian:bookworm-20241016
RUN apt-get update && \
    apt-get install -y g++ libcurl4-openssl-dev libssl-dev git libevent-dev libmysqlcppconn-dev libmariadb-dev make && \
    apt-get clean

    # Создаем папку ~/prod
RUN mkdir -p /home/prod

# Копируем файлы в папку ~/prod
COPY consumer.cpp /home/prod/
COPY mysql-config.json /home/prod/
COPY mq-config.json /home/prod/

# Клонируем библиотеки
RUN mkdir -p /tmp/amqp
RUN mkdir -p /tmp/json

RUN git clone https://github.com/nlohmann/json.git /tmp/json
RUN mkdir -p /usr/local/include/nlohmann/
RUN cp /tmp/json/single_include/nlohmann/json.hpp /usr/local/include/nlohmann/

RUN git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP /tmp/amqp
WORKDIR /tmp/amqp
RUN make 
RUN make install

RUN apt-get update && \
    apt-get install -y gdb && \
    apt-get clean

    RUN apt-get update && \
    apt-get install -y curl && \
    apt-get clean

# Переходим в папку сборки и собираем программу
WORKDIR /home/prod/
RUN g++ consumer.cpp -o consumer -lamqpcpp -levent -lcurl -I/usr/local/include -lmysqlcppconn -g

COPY tt.sh /home/prod/

# Запускаем обработчик в фоновом режиме и выводим логи
CMD ["bash", "tt.sh"]