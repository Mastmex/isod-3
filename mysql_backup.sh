#!/bin/bash

# Настройки
BACKUP_DIR="/var/backups/isod-3-mysql"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")
BACKUP_PATH="$BACKUP_DIR/$TIMESTAMP-mysql-dump"

# Создаем директорию для резервной копии, если её нет
mkdir -p "$BACKUP_PATH"

# Выполняем резервное копирование всех баз данных MySQL
docker exec -i my-mysql mysqldump -u root -pjdui8932erhfASDgsn7823 "STOCKS" > "$BACKUP_PATH/stocks_backup.sql"

# Проверка успешности резервного копирования
if [ $? -eq 0 ]; then
    echo "Резервное копирование успешно завершено: $BACKUP_PATH"
else
    echo "Ошибка резервного копирования MySQL" >&2
fi
