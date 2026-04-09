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

INSERT INTO hotels (id, name, city, address, description, stars) VALUES
('20000000-0000-0000-0000-000000000001', 'Grand Palace Hotel', 'Moscow', 'Tverskaya st. 15', 'Пятизвёздочный отель в самом центре, рядом Кремль и Красная площадь', 5),
('20000000-0000-0000-0000-000000000002', 'Neva View Hotel', 'Saint Petersburg', 'Nevsky Prospekt 45', 'Из окон видно Неву. Завтрак включён, парковки нет', 4),
('20000000-0000-0000-0000-000000000003', 'Kazan Inn', 'Kazan', 'Bauman st. 10', 'Небольшой отель, до Кремля пешком минут 10', 3),
('20000000-0000-0000-0000-000000000004', 'Sochi Beach Resort', 'Sochi', 'Primorskaya st. 25', 'Прямо на берегу, есть бассейн и спа. Летом лучше бронировать заранее', 5),
('20000000-0000-0000-0000-000000000005', 'Moscow Budget Stay', 'Moscow', 'Arbat st. 30', 'Бюджетный вариант на Арбате, комнаты маленькие но чисто', 2),
('20000000-0000-0000-0000-000000000006', 'Peter Hotel', 'Saint Petersburg', 'Liteyny Prospekt 20', 'Современный отель недалеко от центра', 4),
('20000000-0000-0000-0000-000000000007', 'Ural Mountains Lodge', 'Yekaterinburg', 'Lenin st. 50', 'Хороший вид на горы, зимой можно покататься на лыжах рядом', 3),
('20000000-0000-0000-0000-000000000008', 'Volga Riverside Hotel', 'Nizhny Novgorod', 'Rozhdestvenskaya st. 5', 'Стоит прямо над Волгой, номера с видом на реку стоят дороже', 4),
('20000000-0000-0000-0000-000000000009', 'Siberian Comfort', 'Novosibirsk', 'Krasny Prospekt 100', 'Нормальный отель в центре Новосибирска, Wi-Fi быстрый', 3),
('20000000-0000-0000-0000-00000000000a', 'Black Sea Hotel', 'Sochi', 'Kurortniy Prospekt 50', 'Есть свой пляж и спа. Номера с видом на море', 4);
