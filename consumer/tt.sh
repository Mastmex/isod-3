sleep 60    #ожидание запуска rabbit-mq
/home/prod/consumer
while : #поддержка контейнера активным
do
    echo “sup”
    sleep 600
done