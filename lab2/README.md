# Система бронирования отелей (Вариант 13)

Выбрал C++ userver. Страдал, но не жалею :\)\)\)

Всё работает через Docker Compose: три микросервиса + nginx.

## Что реализовано

Все 9 эндпоинтов из задания, плюс refresh/logout для токенов:

| Метод | Путь | Описание | Auth |
|-------|------|----------|------|
| `POST` | `/api/auth/register` | Регистрация пользователя | — |
| `POST` | `/api/auth/login` | Логин -> JWT + refresh-токен | — |
| `POST` | `/api/auth/refresh` | Обновление токенов | — |
| `POST` | `/api/auth/logout` | Выход | — |
| `GET` | `/api/users/by-login/{login}` | Найти пользователя по логину | — |
| `GET` | `/api/users/search?query=...` | Поиск по имени/фамилии | — |
| `POST` | `/api/hotels` | Добавить отель | ✓ |
| `GET` | `/api/hotels[?city=...]` | Список отелей, опционально по городу | — |
| `POST` | `/api/bookings` | Забронировать | ✓ |
| `GET` | `/api/bookings` | Свои бронирования | ✓ |
| `DELETE` | `/api/bookings/{id}` | Отменить бронь | ✓ |

## Стек

- **C++17 + userver** — фреймворк от любимого Яндекса
- **In-memory хранилище** — `std::map` + `std::mutex`, данные живут пока живёт контейнер
- **JWT (HS256)** — access-токен с TTL 24ч + refresh-токен
- **nginx** — API-gateway
- **OpenAPI 3.0** — спека в `openapi.yaml`, Swagger UI поднимается вместе с compose

## Запуск

```bash
# Первый запуск долгий — C++ компилируется ~5-10 минут
docker compose up --build -d

# Сервис доступен на http://localhost:8080
# Swagger UI на http://localhost:8090
```

Чтобы убедиться, что всё поднялось:

```bash
curl http://localhost:8080/ping
```

Если получаете 502 — скорее всего nginx стартовал раньше бэкенда и закешировал старые адреса. Лечится:

```bash
docker compose restart api-gateway
```

## Тесты

```bash
pip install -r tests/test_requirements.txt
pytest tests/ -v --base-url=http://localhost:8080
```

23 теста покрывают  сценарии: регистрацию, логин, ротацию токенов, CRUD отелей и бронирований, обработку ошибок.

## Примеры

### Регистрация и логин

```bash
# Зарегистрироваться
curl -X POST http://localhost:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "login": "alice",
    "password": "qwerty",
    "first_name": "Alice",
    "last_name": "Smith",
    "email": "alice@example.com"
  }'
# -> 201, возвращает данные пользователя

# Залогиниться
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login": "alice", "password": "qwerty"}'
# -> {"token": "<jwt>", "refresh_token": "<uuid>", "user_id": "<uuid>"}
```

### Отели

```bash
# Добавить отель (нужен токен)
curl -X POST http://localhost:8080/api/hotels \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "name": "Ritz",
    "city": "Moscow",
    "address": "Tverskaya 3",
    "description": "5-star hotel",
    "stars": 5
  }'

# Список всех отелей
curl http://localhost:8080/api/hotels

# Список отелей по городу
curl "http://localhost:8080/api/hotels?city=Moscow"
```

### Бронирование

```bash
HOTEL_ID="<id отеля>"

# Забронировать
curl -X POST http://localhost:8080/api/bookings \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d "{\"hotel_id\": \"$HOTEL_ID\", \"check_in\": \"2025-09-01\", \"check_out\": \"2025-09-07\"}"

# Посмотреть свои брони
curl http://localhost:8080/api/bookings \
  -H "Authorization: Bearer $TOKEN"

# Отменить
BOOKING_ID="<id брони>"
curl -X DELETE "http://localhost:8080/api/bookings/$BOOKING_ID" \
  -H "Authorization: Bearer $TOKEN"
```

## Архитектура

Три независимых сервиса, каждый на своём порту внутри docker-сети:

```
nginx :8080 (api-gateway)
├── /api/auth, /api/users  -> auth-service :8081
├── /api/hotels            -> inventory-service :8082
└── /api/bookings          -> booking-service :8083
```

Каждый сервис — отдельный userver-процесс со своим in-memory хранилищем. booking-service ходит в auth-service для проверки токенов и в inventory-service для проверки существования отеля.

Структура исходников каждого сервиса:

```
services/<name>/src/
├── main.cpp          — точка входа, регистрация компонентов
├── storage.hpp/cpp   — in-memory хранилище
└── handlers/         — по файлу на каждый эндпоинт
```

## Аутентификация

При логине возвращаются два токена:
- **JWT (HS256, TTL 24ч)** — передаётся в заголовке `Authorization: Bearer <token>` при каждом запросе
- **Refresh-токен (UUID)** — живёт в памяти сервера, используется только для получения новой пары токенов

При вызове `/api/auth/refresh` старый refresh-токен инвалидируется (ротация) — повторно использовать его нельзя (хз насколько ок такое решение).

## Коды ответов

| Код | Когда |
|-----|-------|
| 200 | Успешный GET или удаление |
| 201 | Ресурс создан |
| 400 | Не хватает полей или неверный формат |
| 401 | Токен отсутствует, истёк или невалиден |
| 404 | Ресурс не найден |
| 409 | Логин занят / бронь уже отменена |

## OpenAPI

Полная спека: [`openapi.yaml`](./openapi.yaml). Swagger UI поднимается автоматически на `http://localhost:8090`.
