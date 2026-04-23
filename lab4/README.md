# Система бронирования отелей — MongoDB (Вариант 13)


## Что изменилось относительно lab3

- `booking-service` теперь использует `userver::storages::mongo` вместо `userver::storages::postgres`
- `booking-db` — теперь MongoDB вместо PostgreSQL
- Timestamp-поля хранятся как `long` (миллисекунды с эпохи) — так userver кладёт `int64_t` в BSON
- Добавлены `mongo-init/` скрипты: создание коллекции с `$jsonSchema`, seed-данные, проверка счётчиков

## Документная модель

Одна коллекция `bookings` в базе `booking_db`. Подробное обоснование embedded vs references — в [`schema_design.md`](./schema_design.md).

Структура документа:
```json
{
  "booking_id": "uuid",
  "user_id":    "uuid",
  "hotel_id":   "uuid",
  "check_in":   "2026-06-01",
  "check_out":  "2026-06-05",
  "status":     "CONFIRMED",
  "nights":     4,
  "created_at": 1776970000000,
  "updated_at": 1776970000000,
  "events": [
    { "type": "CREATED", "timestamp": 1776970000000, "note": "Booking created" }
  ]
}
```

Статусы: `CONFIRMED` (при создании) → `CANCELLED` (при удалении).

## Стек

- **C++17 + userver** с компонентом `userver::storages::mongo`
- **MongoDB 7** — booking-db
- **PostgreSQL 15** — auth-db, inventory-db
- **JWT (HS256)** — аутентификация
- **nginx** — API-gateway
- **Docker Compose** — оркестрация

## Запуск

```bash
# Первый запуск долгий — C++ компилируется ~15 минут
cd lab4
docker compose up --build -d

# API на http://localhost:8080
# Swagger UI на http://localhost:8090
```

При старте MongoDB автоматически выполняет скрипты из `mongo-init/`:
1. `01-validation.js` — создаёт коллекцию с `$jsonSchema` валидатором и индексами
2. `02-data.js` — вставляет 12 тестовых документов
3. `03-queries.js` — выводит счётчики для проверки

```bash
# Проверить что всё поднялось
curl http://localhost:8080/ping
```

Остановка:
```bash
docker compose down      # данные остаются
docker compose down -v   # если хотим ещё и volumes подтереть
```

## API

Эндпоинты те же, что и раньше:

| Метод | Путь | Описание | Auth |
|-------|------|----------|------|
| `POST` | `/api/auth/register` | Регистрация | — |
| `POST` | `/api/auth/login` | Логин → JWT + refresh | — |
| `POST` | `/api/auth/refresh` | Обновить токены | — |
| `POST` | `/api/auth/logout` | Выход | — |
| `GET` | `/api/users/by-login/{login}` | Найти пользователя | — |
| `GET` | `/api/users/search?query=...` | Поиск по имени | — |
| `POST` | `/api/hotels` | Добавить отель | ✓ |
| `GET` | `/api/hotels[?city=...]` | Список отелей | — |
| `POST` | `/api/bookings` | Создать бронь | ✓ |
| `GET` | `/api/bookings` | Мои брони | ✓ |
| `DELETE` | `/api/bookings/{id}` | Отменить бронь | ✓ |

## Скрипты для MongoDB

```bash
# CRUD-запросы + агрегация
docker exec -i booking-db mongosh \
  "mongodb://booking_admin:booking_password@localhost:27017/booking_db?authSource=admin" \
  < queries.js

# Тесты валидации схемы
docker exec -i booking-db mongosh \
  "mongodb://booking_admin:booking_password@localhost:27017/booking_db?authSource=admin" \
  < validation.js

# Пересидить данные (если коллекция была очищена)
docker exec -i booking-db mongosh \
  "mongodb://booking_admin:booking_password@localhost:27017/booking_db?authSource=admin" \
  < data.js
```
