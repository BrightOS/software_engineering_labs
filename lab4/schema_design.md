# Проектирование документной модели (MongoDB)

## Что мигрируем

В системе три сервиса: Auth, Inventory и Booking. Auth и Inventory оставляем на PostgreSQL — там обычные таблицы, нормализованные данные, всё работает. Booking Service переносим на MongoDB, потому что бронь по сути самодостаточный документ, который не нуждается в JOIN-ах и хорошо ложится в документную модель.

## Коллекция `bookings`

Одна коллекция для всего сервиса бронирований.

Структура документа:

```json
{
  "_id": ObjectId,
  "booking_id": "b0000000-0000-0000-0000-000000000001",
  "user_id": "10000000-0000-0000-0000-000000000001",
  "hotel_id": "20000000-0000-0000-0000-000000000001",
  "check_in": "2026-06-01",
  "check_out": "2026-06-05",
  "status": "CONFIRMED",
  "nights": 4,
  "created_at": ISODate("2026-04-13T10:00:00Z"),
  "updated_at": ISODate("2026-04-13T10:00:00Z"),
  "events": [
    {
      "type": "CREATED",
      "timestamp": ISODate("2026-04-13T10:00:00Z"),
      "note": "Booking created"
    }
  ]
}
```

Типы полей:

| Поле | Тип | Описание |
|---|---|---|
| `_id` | ObjectId | автоматически MongoDB |
| `booking_id` | String | UUID, это и есть ID в API |
| `user_id` | String | UUID пользователя |
| `hotel_id` | String | UUID отеля |
| `check_in` / `check_out` | String | дата в формате YYYY-MM-DD |
| `status` | String | CONFIRMED или CANCELLED |
| `nights` | Number | кол-во ночей |
| `created_at` / `updated_at` | Date | временные метки |
| `events` | Array | история изменений брони |

## Embedded vs References

**`events` — embedded (встроенный массив)**

История событий хранится прямо внутри документа брони. Это правильно по нескольким причинам:
- события всегда нужны вместе с бронью, никогда отдельно
- их максимум два (CREATED и CANCELLED) — документ не разрастётся
- атомарно обновляем статус и добавляем событие одним updateOne, не нужны транзакции

**`user_id`, `hotel_id` — references (только ID)**

Храним только UUID, не копируем данные пользователя или отеля внутрь брони. Причины:
- пользователи и отели живут в других сервисах (PostgreSQL), дублировать данные плохо
- если пользователь изменит email или отель поменяет название — нам не нужно ничего перезаписывать в бронях
- при создании брони мы всё равно ходим в inventory-service за проверкой отеля по HTTP, этого достаточно

## Индексы

```javascript
// уникальный по booking_id (по нему ищем в API)
db.bookings.createIndex({ booking_id: 1 }, { unique: true });

// для запроса "все брони пользователя" — основной use case
db.bookings.createIndex({ user_id: 1, created_at: -1 });

// для аналитики по отелям
db.bookings.createIndex({ hotel_id: 1, status: 1 });
```
