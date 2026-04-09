-- Регистрация
INSERT INTO users (login, password, first_name, last_name, email)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, login, first_name, last_name, email;

-- Поиск по логину (используется и при логине, и в API)
SELECT id, login, password, first_name, last_name, email
FROM users
WHERE login = $1;

-- Поиск по маске имени/фамилии
SELECT id, login, first_name, last_name, email
FROM users
WHERE LOWER(first_name || ' ' || last_name) LIKE '%' || LOWER($1) || '%';

-- Проверка пароля при логине
SELECT id
FROM users
WHERE login = $1 AND password = $2;

-- Refresh tokens
INSERT INTO refresh_tokens (token, user_id) VALUES ($1, $2);
SELECT user_id FROM refresh_tokens WHERE token = $1;
DELETE FROM refresh_tokens WHERE token = $1;

-- Создание отеля
INSERT INTO hotels (name, city, address, description, stars)
VALUES ($1, $2, $3, $4, $5)
RETURNING id, name, city, address, description, stars;

-- Список отелей (все / по городу)
SELECT id, name, city, address, description, stars
FROM hotels
ORDER BY created_at DESC;

SELECT id, name, city, address, description, stars
FROM hotels
WHERE city = $1
ORDER BY created_at DESC;

-- Отель по ID (внутренний запрос между сервисами)
SELECT id, name, city, address, description, stars
FROM hotels
WHERE id = $1;

-- Создание бронирования
INSERT INTO bookings (user_id, hotel_id, check_in, check_out)
VALUES ($1, $2, $3, $4)
RETURNING id, user_id, hotel_id, check_in, check_out, status;

-- Бронирования юзера
SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings
WHERE user_id = $1
ORDER BY created_at DESC;

-- Бронирование по ID
SELECT id, user_id, hotel_id, check_in, check_out, status
FROM bookings
WHERE id = $1;

-- Отмена
UPDATE bookings
SET status = 'CANCELLED'
WHERE id = $1 AND status = 'CONFIRMED'
RETURNING id;
