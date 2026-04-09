# Система бронирования отелей — PostgreSQL (Вариант 13)

Перевёл все три сервиса с in-memory на **PostgreSQL 15** через `userver::components::Postgres`

## Схема БД

Три отдельных базы, по одной на сервис:

### Auth DB (`auth_db`)
| Таблица | Колонки | Назначение |
|---------|---------|------------|
| `users` | id (UUID PK), login (UNIQUE), password (SHA-256), first_name, last_name, email, created_at | Пользователи |
| `refresh_tokens` | token (UUID PK), user_id (FK → users), created_at | Refresh-токены |

### Inventory DB (`inventory_db`)
| Таблица | Колонки | Назначение |
|---------|---------|------------|
| `hotels` | id (UUID PK), name, city, address, description, stars (CHECK 0-5), created_at | Каталог отелей |

### Booking DB (`booking_db`)
| Таблица | Колонки | Назначение |
|---------|---------|------------|
| `bookings` | id (UUID PK), user_id, hotel_id, check_in (DATE), check_out (DATE), status, created_at | Бронирования |

Подробности по индексам и оптимизации — в [`optimization.md`](./optimization.md).

## Запуск

```bash
# Первый запуск долгий - C++ компилируется ~5-10 минут
cd lab3
docker compose up --build -d

# API на http://localhost:8080
# Swagger UI на http://localhost:8090
```

При старте PostgreSQL автоматически инициализируется скриптами из `db/` — таблицы создаются и заполняются тестовыми данными (по 10 записей).

```bash
# Проверить что всё поднялось
curl http://localhost:8080/ping
```

Остановка:
```bash
docker compose down       # сервисы вниз, данные остаются
docker compose down -v    # сервисы вниз + удаление volumes с данными
```

## API

Как и в lab2/README.md. Да и в целом оттуда код скопировал. Хотел ограничиться единой директорией с отдельными ветками, да чёт бот не  особо хочет принимать ссылки на ветки.

## Файлы

- `schema.sql` — CREATE TABLE, индексы, ограничения (все три БД в одном файле)
- `data.sql` — тестовые данные
- `queries.sql` — все SQL-запросы для API с удобными плейсхолдерами
- `optimization.md` — обоснование индексов, EXPLAIN ANALYZE, партиционирование
- `db/` — init-скрипты для каждой базы (монтируются в PostgreSQL-контейнеры)
- `services/` — исходники микросервисов (C++ userver + PostgreSQL)

## Стек

- **C++17 + userver** с компонентом `userver::postgresql`
- **PostgreSQL 15** — три инстанса (по одному на сервис)
- **JWT (HS256)** — аутентификация
- **nginx** — API-gateway
- **Docker Compose** — оркестрация всего зоопарка
