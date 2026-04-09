import pytest
import pytest_asyncio
import aiohttp
from datetime import date, timedelta


def pytest_addoption(parser):
    parser.addoption("--base-url", default="http://localhost:8080")


@pytest.fixture(scope="session")
def base_url(pytestconfig):
    return pytestconfig.getoption("--base-url")


@pytest_asyncio.fixture(scope="session")
async def http_session():
    async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=30)) as session:
        yield session


@pytest_asyncio.fixture(scope="session")
async def login_data(http_session, base_url):
    r = await http_session.post(f"{base_url}/api/auth/register", json={
        "login": "testuser",
        "password": "pass123",
        "first_name": "Test",
        "last_name": "User",
        "email": "test@example.com",
    })
    assert r.status in (201, 409)

    r = await http_session.post(f"{base_url}/api/auth/login", json={
        "login": "testuser",
        "password": "pass123",
    })
    assert r.status == 200
    return await r.json()


@pytest.fixture(scope="session")
def auth_token(login_data):
    return login_data["token"]


@pytest_asyncio.fixture(scope="session")
async def hotel_id(http_session, base_url, auth_token):
    r = await http_session.post(f"{base_url}/api/hotels", json={
        "name": "Grand Hotel",
        "city": "Moscow",
        "address": "Tverskaya 1",
        "description": "Luxury",
        "stars": 5,
    }, headers={"Authorization": f"Bearer {auth_token}"})
    assert r.status == 201
    return (await r.json())["id"]


@pytest_asyncio.fixture(scope="session")
async def booking_id(http_session, base_url, auth_token, hotel_id):
    check_in = date.today() + timedelta(days=30)
    check_out = check_in + timedelta(days=6)
    r = await http_session.post(f"{base_url}/api/bookings", json={
        "hotel_id": hotel_id,
        "check_in": check_in.isoformat(),
        "check_out": check_out.isoformat(),
    }, headers={"Authorization": f"Bearer {auth_token}"})
    assert r.status == 201
    return (await r.json())["id"]
