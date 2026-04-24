# Система бронирования отелей - Кеширование и Rate Limiting (Вариант 13)

## Что изменилось

1. **In-memory кеширование** в `inventory-service` для двух endpoints
2. **Token Bucket rate limiting** в `booking-service` для `POST /api/bookings`
3. **Prometheus + Grafana** для мониторинга кастомных и встроенных userver-метрик

Подробный перфоманс дезигн - в [`performance_design.md`](./performance_design.md).

## Стек

- **C++17 + userver** - три базовых сервиса
- **PostgreSQL 15** - auth-db, inventory-db
- **MongoDB 7** - booking-db
- **Prometheus** - сбор метрик
- **Grafana** - красивая визуализация метрик
- **nginx** - API-gateway
- **Docker Compose** - оркестрация

## Запуск

```bash
# Первый запуск долгий — C++ компилируется ~15 минут
cd lab4
docker compose up --build -d
```

| Сервис | URL |
|---|---|
| API (через nginx) | http://localhost:8080 |
| Swagger UI | http://localhost:8090 |
| Prometheus | http://localhost:9090 |
| Grafana | http://localhost:3000 (admin/admin) |
| auth-service monitor | http://localhost:8091/service/monitor?format=prometheus-untyped |
| inventory-service monitor | http://localhost:8092/service/monitor?format=prometheus-untyped |
| booking-service monitor | http://localhost:8093/service/monitor?format=prometheus-untyped |

## API

| Метод | Путь | Описание | Auth |
|-------|------|----------|------|
| `POST` | `/api/auth/register` | Регистрация | - |
| `POST` | `/api/auth/login` | Логин → JWT + refresh | - |
| `POST` | `/api/auth/refresh` | Обновить токены | - |
| `POST` | `/api/auth/logout` | Выход | - |
| `GET` | `/api/users/by-login/{login}` | Найти пользователя | - |
| `GET` | `/api/users/search?query=...` | Поиск по имени | - |
| `POST` | `/api/hotels` | Добавить отель | ✓ |
| `GET` | `/api/hotels[?city=...]` | Список отелей (**кешируется**) | - |
| `POST` | `/api/bookings` | Создать бронь (**rate limited**) | ✓ |
| `GET` | `/api/bookings` | Мои брони | ✓ |
| `DELETE` | `/api/bookings/{id}` | Отменить бронь | ✓ |

## Кеширование

Реализовано Cache-Aside в `StorageComponent` (inventory-service):

- **`GET /api/hotels[?city=X]`** - TTL **5 минут**, инвалидируется при `POST /api/hotels`
- **`GET /api/internal/hotels/{id}`** - TTL **10 минут**, без инвалидации (отели иммутабельны)

Чтоб затриггерить **`GET /api/internal/hotels/{id}`**, можно отправить запрос бронирования отеля **`POST /api/bookings/`**.

Попадания/промахи экспортируются в Prometheus:
```
inventory_cache_list_hits / inventory_cache_list_misses
inventory_cache_hotel_hits / inventory_cache_hotel_misses
```

## Rate Limiting

`POST /api/bookings` ограничен алгоритмом **Token Bucket**:
- 10 запросов в первую минуту
- Пополнение: 1 токен каждые 6 секунд
- Ключ: `user_id` из JWT

При превышении лимита:
```http
HTTP/1.1 429 Too Many Requests
X-RateLimit-Limit: 10
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 18
```

Счётчики `booking_rate_limiter_allowed` и `booking_rate_limiter_rejected` доступны в Prometheus.

## Метрики

Все три сервиса выставляют метрики через userver `ServerMonitor`

Prometheus обновляет их каждые 15 секунд. Grafana получает данные из Prometheus автоматически.

### Grafana

Доступен по ссылке - **http://localhost:3000** (admin/admin). Там надо будет найти **"Дашборд бронирования отелей"**.

Дашборд разбит на три блока по запросам:

**GET /api/hotels — список отелей**
- Сколько раз список отдали из кеша, сколько раз пришлось идти в PostgreSQL
- Gauge с процентом попаданий (красный < 50%, жёлтый 50–80%, зелёный ≥ 80%)
- Таймсерия: кеш vs база в реальном времени

**GET /api/internal/hotels/{id} — проверка отеля при бронировании**
- Тот же набор, но для кеша по ID отеля. Этот запрос не вызывается напрямую — он срабатывает внутри при каждом `POST /api/bookings`

**POST /api/bookings — создание бронирования**
- Сколько запросов пропущено, сколько заблокировано с кодом 429
- Gauge с долей заблокированных (зелёный < 10%, жёлтый 10–30%, красный >= 30%)
- Таймсерия: пропущено vs заблокировано в реальном времени

## Проверка rate limiting вручную

```bash
TOKEN=$(curl -s -X POST http://localhost:8080/api/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"alex.ivanov","password":"password123"}' | jq -r .token)

for i in $(seq 1 12); do
  STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
    -X POST http://localhost:8080/api/bookings \
    -H "Authorization: Bearer $TOKEN" \
    -H 'Content-Type: application/json' \
    -d '{"hotel_id":"20000000-0000-0000-0000-000000000001","check_in":"2026-07-01","check_out":"2026-07-03"}')
  echo "Request $i: $STATUS"
done
```

11-й и 12-й запросы должны вернуть 429
