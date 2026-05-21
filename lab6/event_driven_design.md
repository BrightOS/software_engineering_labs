# Event-Driven Design

## 1. Какие события есть в системе

Систему бронирования отелей посмотрел с точки зрения того, что в ней реально происходит. Выделил два типа изменений, о которых другие компоненты должны знать - создание брони и отмена брони. Всё остальное (регистрация, добавление отеля) пока не требует асинхронного распространения, потому что нет подписчиков.

Команды, которые инициируют события:
- `CreateBooking` - клиент создаёт бронь через `POST /api/bookings`
- `CancelBooking` - клиент отменяет бронь через `DELETE /api/bookings/{id}`

Каждая команда при успехе порождает одно событие. Если команда не выполнилась (отель не найден, бронь уже отменена), событие не публикуется.

---

## 2. Кто производит и кто потребляет

**Производитель** один - `booking-service`. Именно он меняет состояние брони и знает, что произошло. Публикацию вынес в отдельный компонент `EventPublisherComponent`, чтобы не смешивать HTTP-логику с брокером.

**Потребитель** - `mailer-service`. Он подписан на все события бронирования и отправляет уведомления клиентам. В текущей реализации вместо реального SMTP - логирование, но структурно это полноценный worker.

Поток данных выглядит так:

```
Клиент
  -> POST /api/bookings
booking-service
  -> сохраняет бронь в MongoDB
  -> публикует booking.created -> RabbitMQ
                                      -> mailer-service
                                      -> [MAILER] Sending confirmation email...
```

---

## 3. Брокер и формат сообщений

Выбрал **RabbitMQ** - в userver есть нативный компонент `urabbitmq` с нормальным C++ API, и поднять его проще, чем Kafka. Архитектурно в схеме L1 был Kafka, но для этой лабораторной разницы нет: оба поддерживают topic-маршрутизацию и at-least-once.

Топология:
- Exchange `booking_events` типа `topic` - оба сервиса объявляют его в конструкторе, RabbitMQ идемпотентен к повторным DeclareExchange
- Queue `mailer_queue` с binding `booking.*` - маршрутизирует и `booking.created`, и `booking.cancelled`

Формат сообщений - JSON. Пример для `booking.created`:

```json
{
  "event_type": "booking.created",
  "booking_id": "550e8400-e29b-41d4-a716-446655440000",
  "user_id": "b2c3d4e5-f6a7-48b9-c0d1-e2f3a4b5c6d7",
  "hotel_id": "a1b2c3d4-e5f6-47a8-b9c0-d1e2f3a4b5c6",
  "check_in": "2026-06-01",
  "check_out": "2026-06-05",
  "status": "CONFIRMED"
}
```

Для `booking.cancelled` нет смысла тащить все данные - только `booking_id` и `user_id`, потому что mailer-service больше ничего не нужно для письма об отмене.

**Гарантии доставки**: at-least-once. userver публикует с publisher confirms - если подтверждения не пришло, при следующем шансе опубликует снова. На стороне consumer `ConsumerComponentBase` использует manual ack - сообщение подтверждается только после того, как `Process()` вернул управление без исключения. Дублей быть может, но для email-уведомлений это некритично.

---

## 4. CQRS

Частично применимо в `booking-service`. Разделение уже есть:

- **Write-path**: `POST /api/bookings` и `DELETE /api/bookings/{id}` - меняют состояние в MongoDB и публикуют событие
- **Read-path**: `GET /api/bookings` - читает список броней пользователя

Сейчас write и read ходят в одну MongoDB-коллекцию, то есть read model и write model совпадают. Разделить их на отдельные хранилища можно через события: при получении `booking.created` read-model-updater вставляет запись в read-store, при `booking.cancelled` - обновляет статус. Пока такой сервис не реализован, но место для него в архитектуре есть.

---

## 5. Реализация

Изменения в `booking-service`:

- Новый `EventPublisherComponent` (`event_publisher.hpp/cpp`) - инициализирует exchange и держит `shared_ptr<urabbitmq::Client>`
- `BookingHandler::HandlePost` вызывает `publisher_.PublishBookingCreated(b)` после `storage_.AddBooking()`
- `BookingCancelHandler::HandleRequestThrow` вызывает `publisher_.PublishBookingCancelled()` после `storage_.CancelBooking()`
- Ошибки публикации перехватываются и логируются - HTTP-ответ клиенту не зависит от состояния брокера

Новый `mailer-service`:

- Основан на `ConsumerComponentBase` из userver
- В конструкторе объявляет exchange, queue, binding
- `Process(message)` разбирает JSON, определяет тип события и логирует нужный шаблон письма
