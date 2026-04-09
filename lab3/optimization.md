# Оптимизация запросов

Здесь описано какие индексы были добавлены и почему, плюс EXPLAIN ANALYZE для основных запросов.

## Индексы

Все индексы B-Tree, создаются в init-скриптах при старте контейнеров.

**Auth DB:**
- `idx_users_login` — по логину ищем при каждом входе, без индекса будет seq scan
- `idx_users_name_lower` — составной по `LOWER(first_name), LOWER(last_name)`, для поиска по маске. Честно говоря, для `LIKE '%..%'` он не помогает (нужен pg_trgm), но для точного совпадения по имени работает
- `idx_refresh_tokens_user_id` — чтобы при удалении юзера каскадно удалялись токены быстро

**Inventory DB:**
- `idx_hotels_city` — фильтрация по городу, самый частый запрос к отелям
- `idx_hotels_stars` — фильтрация по звёздам (пока не используется в API, но может пригодиться)

**Booking DB:**
- `idx_bookings_user_id` — получение бронирований юзера
- `idx_bookings_hotel_id` — получение бронирований по отелю
- `idx_bookings_status` — фильтрация по статусу при отмене
- `idx_bookings_check_in` — для сортировки/фильтрации по дате заезда

## EXPLAIN ANALYZE

Запускал на тестовых данных (по 10 записей), поэтому времена маленькие, но видно что индексы подхватываются.

### Поиск по логину

```sql
EXPLAIN ANALYZE SELECT id, login, password, first_name, last_name, email
FROM users WHERE login = 'ivan.petrov';
```

```
Index Scan using idx_users_login on users  (cost=0.14..8.16 rows=1 width=200) (actual time=0.020..0.021 rows=1 loops=1)
  Index Cond: ((login)::text = 'ivan.petrov'::text)
Planning Time: 0.095 ms
Execution Time: 0.038 ms
```

Используется `idx_users_login`, всё ок.

### Бронирования юзера

```sql
EXPLAIN ANALYZE SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings WHERE user_id = '10000000-0000-0000-0000-000000000001'::uuid
ORDER BY created_at DESC;
```

```
Index Scan using idx_bookings_user_id on bookings  (cost=0.14..8.17 rows=2 width=120) (actual time=0.012..0.014 rows=2 loops=1)
  Index Cond: (user_id = '10000000-0000-0000-0000-000000000001'::uuid)
Planning Time: 0.084 ms
Execution Time: 0.030 ms
```

Index Scan по `idx_bookings_user_id`. Сортировка по `created_at` идёт в памяти (строк мало), но если данных станет много — можно сделать составной индекс `(user_id, created_at DESC)`.

### Отели по городу

```sql
EXPLAIN ANALYZE SELECT id, name, city, address, description, stars
FROM hotels WHERE city = 'Moscow' ORDER BY created_at DESC;
```

```
Index Scan using idx_hotels_city on hotels  (cost=0.14..8.17 rows=2 width=250) (actual time=0.015..0.017 rows=2 loops=1)
  Index Cond: ((city)::text = 'Moscow'::text)
Planning Time: 0.070 ms
Execution Time: 0.032 ms
```

Работает через `idx_hotels_city`.

### Поиск юзеров по маске имени

```sql
EXPLAIN ANALYZE SELECT id, login, first_name, last_name, email
FROM users WHERE LOWER(first_name || ' ' || last_name) LIKE '%ivan%';
```

```
Seq Scan on users  (cost=0.00..1.38 rows=1 width=168) (actual time=0.020..0.025 rows=1 loops=1)
  Filter: (lower(((first_name)::text || ' '::text) || (last_name)::text) ~~ '%ivan%'::text)
  Rows Removed by Filter: 9
Planning Time: 0.075 ms
Execution Time: 0.035 ms
```

Тут Seq Scan — B-Tree не умеет в `LIKE '%...%'`. На 10 записях это не проблема, но на больших таблицах надо бы поставить `pg_trgm` и GIN-индекс. Пока не стал этого делать, потому что расширение надо отдельно подключать в PostgreSQL.

## Партиционирование

Таблица `bookings` будет расти быстрее всех — каждое бронирование это новая строка. Можно разбить по дате заезда (range partitioning по `check_in`):

```sql
CREATE TABLE bookings_partitioned (
    id          UUID DEFAULT gen_random_uuid(),
    user_id     UUID NOT NULL,
    hotel_id    UUID NOT NULL,
    check_in    DATE NOT NULL,
    check_out   DATE NOT NULL,
    status      VARCHAR(20) NOT NULL DEFAULT 'CONFIRMED',
    created_at  TIMESTAMP NOT NULL DEFAULT NOW(),
    PRIMARY KEY (id, check_in),
    CONSTRAINT chk_dates CHECK (check_out > check_in),
    CONSTRAINT chk_status CHECK (status IN ('CONFIRMED', 'CANCELLED'))
) PARTITION BY RANGE (check_in);

-- по кварталам, новые добавляются по мере необходимости
CREATE TABLE bookings_2026_q1 PARTITION OF bookings_partitioned
    FOR VALUES FROM ('2026-01-01') TO ('2026-04-01');
CREATE TABLE bookings_2026_q2 PARTITION OF bookings_partitioned
    FOR VALUES FROM ('2026-04-01') TO ('2026-07-01');
```

Плюсы: старые партиции можно дропнуть целиком вместо DELETE, и PostgreSQL при запросе автоматически отсекает ненужные партиции (partition pruning). В текущей реализации партиционирование не включено, т.к. на 10 записях смысла нет.
