# Каталог событий

Система публикует два события, оба из `booking-service`. Брокер - RabbitMQ, exchange `booking_events` типа topic.

---

## booking.created

Публикуется после успешного создания брони в MongoDB.

**Производитель**: `booking-service`  
**Потребители**: `mailer-service`  
**Routing key**: `booking.created`  
**Гарантии**: at-least-once (publisher confirms + manual consumer ack)

Payload:
```json
{
  "event_type": "booking.created",
  "booking_id": "<UUID>",
  "user_id": "<UUID>",
  "hotel_id": "<UUID>",
  "check_in": "YYYY-MM-DD",
  "check_out": "YYYY-MM-DD",
  "status": "CONFIRMED"
}
```

`mailer-service` при получении логирует:
```
[MAILER] Sending confirmation email | user=<id> booking=<id> hotel=<id> dates=<check_in>/<check_out>
```

---

## booking.cancelled

Публикуется после успешной отмены брони - только если бронь до этого была в статусе `CONFIRMED`. Если бронь уже отменена, событие не публикуется.

**Производитель**: `booking-service`  
**Потребители**: `mailer-service`  
**Routing key**: `booking.cancelled`  
**Гарантии**: at-least-once (publisher confirms + manual consumer ack)

Payload намеренно минимальный - mailer-service для письма об отмене не нужны даты или отель, достаточно знать кому и о чём писать:
```json
{
  "event_type": "booking.cancelled",
  "booking_id": "<UUID>",
  "user_id": "<UUID>"
}
```

`mailer-service` при получении логирует:
```
[MAILER] Sending cancellation email | user=<id> booking=<id>
```

---

## Топология брокера

```
booking_events (topic exchange)
  booking.created  -> mailer_queue -> mailer-service
  booking.cancelled -> mailer_queue -> mailer-service
```

Binding key `booking.*` покрывает оба события. Если в будущем добавится `booking.modified` или `booking.paid` - mailer_queue начнёт получать их автоматически без изменений в инфраструктуре.
