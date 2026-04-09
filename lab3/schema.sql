-- auth

CREATE TABLE IF NOT EXISTS users (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    login       VARCHAR(255) UNIQUE NOT NULL,
    password    VARCHAR(64) NOT NULL, -- sha-256 hex
    first_name  VARCHAR(255) NOT NULL,
    last_name   VARCHAR(255) NOT NULL,
    email       VARCHAR(255),
    created_at  TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS refresh_tokens (
    token       UUID PRIMARY KEY,
    user_id     UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    created_at  TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_users_login              ON users(login);
CREATE INDEX IF NOT EXISTS idx_users_name_lower         ON users(LOWER(first_name), LOWER(last_name));
CREATE INDEX IF NOT EXISTS idx_refresh_tokens_user_id   ON refresh_tokens(user_id);

-- hotels
CREATE TABLE IF NOT EXISTS hotels (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name        VARCHAR(255) NOT NULL,
    city        VARCHAR(255) NOT NULL,
    address     VARCHAR(500) NOT NULL,
    description TEXT DEFAULT '',
    stars       INTEGER NOT NULL DEFAULT 0 CHECK (stars >= 0 AND stars <= 5),
    created_at  TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_hotels_city  ON hotels(city);
CREATE INDEX IF NOT EXISTS idx_hotels_stars ON hotels(stars);

-- bookings
CREATE TABLE IF NOT EXISTS bookings (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id     UUID NOT NULL,
    hotel_id    UUID NOT NULL,
    check_in    DATE NOT NULL,
    check_out   DATE NOT NULL,
    status      VARCHAR(20) NOT NULL DEFAULT 'CONFIRMED',
    created_at  TIMESTAMP NOT NULL DEFAULT NOW(),
    CONSTRAINT chk_dates CHECK (check_out > check_in),
    CONSTRAINT chk_status CHECK (status IN ('CONFIRMED', 'CANCELLED'))
);

CREATE INDEX IF NOT EXISTS idx_bookings_user_id     ON bookings(user_id);
CREATE INDEX IF NOT EXISTS idx_bookings_hotel_id    ON bookings(hotel_id);
CREATE INDEX IF NOT EXISTS idx_bookings_status      ON bookings(status);
CREATE INDEX IF NOT EXISTS idx_bookings_check_in    ON bookings(check_in);
