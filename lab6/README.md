# Система бронирования отелей - Event-Driven архитектура (Вариант 13)

## Что изменилось

1. **RabbitMQ** как брокер событий
2. **booking-service** теперь публикует события `booking.created` и `booking.cancelled` после каждой операции с бронью
3. **mailer-service** - новый C++ userver-worker, подписан на все события бронирования и имитирует отправку email-уведомлений

Подробный event-driven дезигн - в [`event_driven_design.md`](./event_driven_design.md). Каталог событий - в [`event_catalog.md`](./event_catalog.md).

## Стек

- **C++17 + userver** - три исходных сервиса + новый mailer-service
- **PostgreSQL 15** - auth-db, inventory-db
- **MongoDB 7** - booking-db
- **RabbitMQ 3** - брокер событий
- **nginx** - API-gateway
- **Docker Compose** - оркестрация

## Запуск

```bash
cd lab6
make up
```

Первый запуск долгий - C++ компилируется ~15-20 минут.

| Сервис | URL |
|---|---|
| API (через nginx) | http://localhost:8080 |
| Swagger UI | http://localhost:8090 |
| RabbitMQ Management | http://localhost:15672 (guest/guest) |
| booking-service monitor | http://localhost:8093/service/monitor |
| mailer-service monitor | http://localhost:8094/service/monitor |

## API

| Метод | Путь | Описание | Auth |
|-------|------|----------|------|
| `POST` | `/api/auth/register` | Регистрация | - |
| `POST` | `/api/auth/login` | Логин -> JWT + refresh | - |
| `POST` | `/api/auth/refresh` | Обновить токены | - |
| `POST` | `/api/auth/logout` | Выход | - |
| `GET` | `/api/users/by-login/{login}` | Найти пользователя | - |
| `GET` | `/api/users/search?query=...` | Поиск по имени | - |
| `POST` | `/api/hotels` | Добавить отель | ✓ |
| `GET` | `/api/hotels[?city=...]` | Список отелей | - |
| `POST` | `/api/bookings` | Создать бронь (**+ событие**) | ✓ |
| `GET` | `/api/bookings` | Мои брони | ✓ |
| `DELETE` | `/api/bookings/{id}` | Отменить бронь (**+ событие**) | ✓ |

## Проверка событий

Полный сценарий одним скриптом (нужен `jq`):

```bash
# Регистрация
curl -s -X POST http://localhost:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"login":"alice","password":"secret","first_name":"Alice","last_name":"Smith","email":"alice@example.com"}'

# Логин -> сохраняем токен
TOKEN=$(curl -s -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"login":"alice","password":"secret"}' | jq -r .token)

# Создаём отель -> сохраняем id
HOTEL_ID=$(curl -s -X POST http://localhost:8080/api/hotels \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"name":"Grand Hotel","city":"Moscow","address":"Tverskaya 1","stars":5}' | jq -r .id)

# Создаём бронь -> сохраняем id
BOOKING_ID=$(curl -s -X POST http://localhost:8080/api/bookings \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"hotel_id\":\"$HOTEL_ID\",\"check_in\":\"2026-07-01\",\"check_out\":\"2026-07-05\"}" | jq -r .id)

echo "Booking: $BOOKING_ID"
docker logs mailer-service 2>&1 | grep MAILER

# Отменяем бронь
curl -s -X DELETE http://localhost:8080/api/bookings/$BOOKING_ID \
  -H "Authorization: Bearer $TOKEN"

docker logs mailer-service 2>&1 | grep MAILER
```

Очереди и exchange можно посмотреть в RabbitMQ Management UI: **http://localhost:15672** - вкладка Queues -> `mailer_queue`.
