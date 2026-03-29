import jwt


JWT_SECRET = "hotel-booking-secret-key-lab2"


async def test_ping(http_session, base_url):
    r = await http_session.get(f"{base_url}/ping")
    assert r.status == 200


class TestAuth:
    async def test_register_duplicate(self, http_session, base_url, login_data):
        r = await http_session.post(f"{base_url}/api/auth/register", json={
            "login": "testuser",
            "password": "pass123",
            "first_name": "Test",
            "last_name": "User",
        })
        assert r.status == 409

    async def test_register_missing_fields(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/register", json={"login": "x"})
        assert r.status == 400

    async def test_login_returns_jwt(self, auth_token):
        payload = jwt.decode(auth_token, JWT_SECRET, algorithms=["HS256"])
        assert payload.get("iss") == "hotel-booking"
        assert "sub" in payload
        assert "exp" in payload

    async def test_login_returns_refresh_token(self, login_data):
        assert "refresh_token" in login_data
        assert login_data["refresh_token"]

    async def test_login_wrong_password(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/login", json={
            "login": "testuser",
            "password": "wrong",
        })
        assert r.status == 401

    async def test_refresh_returns_new_tokens(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/login", json={
            "login": "testuser",
            "password": "pass123",
        })
        assert r.status == 200
        data = await r.json()
        old_refresh = data["refresh_token"]

        r = await http_session.post(f"{base_url}/api/auth/refresh", json={
            "refresh_token": old_refresh,
        })
        assert r.status == 200
        new_data = await r.json()
        assert "token" in new_data
        assert "refresh_token" in new_data
        assert new_data["refresh_token"] != old_refresh
        payload = jwt.decode(new_data["token"], JWT_SECRET, algorithms=["HS256"])
        assert payload.get("iss") == "hotel-booking"

    async def test_refresh_old_token_invalid_after_rotation(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/login", json={
            "login": "testuser",
            "password": "pass123",
        })
        data = await r.json()
        old_refresh = data["refresh_token"]

        await http_session.post(f"{base_url}/api/auth/refresh", json={
            "refresh_token": old_refresh,
        })

        r = await http_session.post(f"{base_url}/api/auth/refresh", json={
            "refresh_token": old_refresh,
        })
        assert r.status == 401

    async def test_refresh_invalid_token(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/refresh", json={
            "refresh_token": "00000000-0000-0000-0000-000000000000",
        })
        assert r.status == 401

    async def test_refresh_missing_field(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/refresh", json={})
        assert r.status == 400

    async def test_logout(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/login", json={
            "login": "testuser",
            "password": "pass123",
        })
        data = await r.json()
        token = data["refresh_token"]

        r = await http_session.post(f"{base_url}/api/auth/logout", json={
            "refresh_token": token,
        })
        assert r.status == 200

        r = await http_session.post(f"{base_url}/api/auth/refresh", json={
            "refresh_token": token,
        })
        assert r.status == 401

    async def test_logout_invalid_token(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/auth/logout", json={
            "refresh_token": "00000000-0000-0000-0000-000000000000",
        })
        assert r.status == 404


class TestUsers:
    async def test_find_by_login(self, http_session, base_url, auth_token):
        r = await http_session.get(f"{base_url}/api/users/by-login/testuser")
        assert r.status == 200

    async def test_find_by_login_not_found(self, http_session, base_url):
        r = await http_session.get(f"{base_url}/api/users/by-login/nobody")
        assert r.status == 404

    async def test_search(self, http_session, base_url):
        r = await http_session.get(f"{base_url}/api/users/search", params={"query": "Test"})
        assert r.status == 200
        data = await r.json()
        assert isinstance(data, list)

    async def test_search_no_query(self, http_session, base_url):
        r = await http_session.get(f"{base_url}/api/users/search")
        assert r.status == 400


class TestHotels:
    async def test_create_no_auth(self, http_session, base_url):
        r = await http_session.post(f"{base_url}/api/hotels", json={
            "name": "Test",
            "city": "SPb",
            "address": "Nevsky 1",
        })
        assert r.status == 401

    async def test_list(self, http_session, base_url, hotel_id):
        r = await http_session.get(f"{base_url}/api/hotels")
        assert r.status == 200
        data = await r.json()
        assert any(h["id"] == hotel_id for h in data)

    async def test_filter_by_city(self, http_session, base_url, hotel_id):
        r = await http_session.get(f"{base_url}/api/hotels", params={"city": "Moscow"})
        assert r.status == 200
        data = await r.json()
        assert any(h["id"] == hotel_id for h in data)


class TestBookings:
    async def test_create_unknown_hotel(self, http_session, base_url, auth_token):
        r = await http_session.post(f"{base_url}/api/bookings", json={
            "hotel_id": "00000000-0000-0000-0000-000000000000",
            "check_in": "2025-08-01",
            "check_out": "2025-08-07",
        }, headers={"Authorization": f"Bearer {auth_token}"})
        assert r.status == 404

    async def test_list(self, http_session, base_url, auth_token, booking_id):
        r = await http_session.get(f"{base_url}/api/bookings",
                                   headers={"Authorization": f"Bearer {auth_token}"})
        assert r.status == 200
        data = await r.json()
        assert any(b["id"] == booking_id for b in data)

    async def test_cancel(self, http_session, base_url, auth_token, booking_id):
        r = await http_session.delete(f"{base_url}/api/bookings/{booking_id}",
                                      headers={"Authorization": f"Bearer {auth_token}"})
        assert r.status == 200

    async def test_cancel_already_cancelled(self, http_session, base_url, auth_token, booking_id):
        r = await http_session.delete(f"{base_url}/api/bookings/{booking_id}",
                                      headers={"Authorization": f"Bearer {auth_token}"})
        assert r.status == 409
