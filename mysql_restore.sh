#!/bin/bash

# Настройки
BACKUP_DIR="/var/backups/isod-3-mysql"
LATEST_BACKUP=$(ls -td "$BACKUP_DIR"/* | head -1)
DUMP_FILE="$LATEST_BACKUP/stocks_backup.sql"  # Укажите файл, если сохраняли только одну БД, измените это имя

# Проверка существования резервной копии
if [[ ! -f "$DUMP_FILE" ]]; then
    echo "Резервная копия не найдена: $DUMP_FILE"
    exit 1
fi

# Восстановление базы данных из резервной копии
echo "Восстановление базы данных из файла: $DUMP_FILE"
docker exec -i my-mysql mysql -u root -pjdui8932erhfASDgsn7823 STOCKS < "$DUMP_FILE"

# Проверка успешности восстановления
if [ $? -eq 0 ]; then
    echo "Восстановление базы данных успешно завершено."
else
    echo "Ошибка восстановления базы данных." >&2
fi
