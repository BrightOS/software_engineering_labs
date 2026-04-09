-- Тестовый пароль: password123 / 123456
-- SHA-256('password123') = ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f
-- SHA-256('123456') = a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3

CREATE TABLE IF NOT EXISTS users (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    login       VARCHAR(255) UNIQUE NOT NULL,
    password    VARCHAR(64) NOT NULL,
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

INSERT INTO users (id, login, password, first_name, last_name, email) VALUES
('10000000-0000-0000-0000-000000000001', 'ivan.petrov', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Ivan', 'Petrov', 'ivan.petrov@mail.ru'),
('10000000-0000-0000-0000-000000000002', 'maria.sidorova', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Maria', 'Sidorova', 'maria.s@gmail.com'),
('10000000-0000-0000-0000-000000000003', 'alex.ivanov', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Alexander', 'Ivanov', 'alex.ivanov@yandex.ru'),
('10000000-0000-0000-0000-000000000004', 'elena.kozlova', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Elena', 'Kozlova', 'elena.k@mail.ru'),
('10000000-0000-0000-0000-000000000005', 'dmitry.novikov', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Dmitry', 'Novikov', 'dmitry.n@gmail.com'),
('10000000-0000-0000-0000-000000000006', 'anna.smirnova', 'a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3', 'Anna', 'Smirnova', 'anna.sm@yandex.ru'),
('10000000-0000-0000-0000-000000000007', 'sergey.volkov', 'a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3', 'Sergey', 'Volkov', 'sergey.v@mail.ru'),
('10000000-0000-0000-0000-000000000008', 'olga.morozova', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Olga', 'Morozova', 'olga.m@gmail.com'),
('10000000-0000-0000-0000-000000000009', 'nikolay.popov', 'a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3', 'Nikolay', 'Popov', 'nikolay.p@yandex.ru'),
('10000000-0000-0000-0000-00000000000a', 'tatiana.sokolova', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'Tatiana', 'Sokolova', 'tatiana.s@mail.ru');
