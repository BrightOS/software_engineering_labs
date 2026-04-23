db = db.getSiblingDB("booking_db");

var nowMs = new Date().getTime();

function daysAgo(n) {
    return NumberLong(nowMs - n * 86400000);
}

// uuid из auth-init.sql
var u1 = "10000000-0000-0000-0000-000000000001"; // ivan.petrov
var u2 = "10000000-0000-0000-0000-000000000002"; // maria.sidorova
var u3 = "10000000-0000-0000-0000-000000000003"; // alex.ivanov
var u4 = "10000000-0000-0000-0000-000000000004"; // elena.kozlova
var u5 = "10000000-0000-0000-0000-000000000005"; // dmitry.novikov

// uuid из inventory-init.sql
var h1 = "20000000-0000-0000-0000-000000000001"; // Grand Palace Hotel, Moscow, 5*
var h2 = "20000000-0000-0000-0000-000000000002"; // Neva View Hotel, SPb, 4*
var h3 = "20000000-0000-0000-0000-000000000003"; // Kazan Inn, 3*
var h4 = "20000000-0000-0000-0000-000000000004"; // Sochi Beach Resort, 5*
var h6 = "20000000-0000-0000-0000-000000000006"; // Peter Hotel, SPb, 4*

db.bookings.insertMany([
    {
        booking_id: "b0000000-0000-0000-0000-000000000001",
        user_id: u1, hotel_id: h1,
        check_in: "2026-06-01", check_out: "2026-06-05",
        status: "CONFIRMED", nights: 4,
        created_at: daysAgo(10), updated_at: daysAgo(10),
        events: [{ type: "CREATED", timestamp: daysAgo(10), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000002",
        user_id: u1, hotel_id: h4,
        check_in: "2026-08-10", check_out: "2026-08-17",
        status: "CONFIRMED", nights: 7,
        created_at: daysAgo(5), updated_at: daysAgo(5),
        events: [{ type: "CREATED", timestamp: daysAgo(5), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000003",
        user_id: u2, hotel_id: h2,
        check_in: "2026-05-15", check_out: "2026-05-18",
        status: "CANCELLED", nights: 3,
        created_at: daysAgo(30), updated_at: daysAgo(20),
        events: [
            { type: "CREATED",   timestamp: daysAgo(30), note: "Booking created" },
            { type: "CANCELLED", timestamp: daysAgo(20), note: "Booking cancelled by user" }
        ]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000004",
        user_id: u2, hotel_id: h3,
        check_in: "2026-03-01", check_out: "2026-03-03",
        status: "CONFIRMED", nights: 2,
        created_at: daysAgo(60), updated_at: daysAgo(60),
        events: [{ type: "CREATED", timestamp: daysAgo(60), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000005",
        user_id: u3, hotel_id: h1,
        check_in: "2026-07-20", check_out: "2026-07-25",
        status: "CONFIRMED", nights: 5,
        created_at: daysAgo(3), updated_at: daysAgo(3),
        events: [{ type: "CREATED", timestamp: daysAgo(3), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000006",
        user_id: u3, hotel_id: h4,
        check_in: "2026-12-20", check_out: "2027-01-03",
        status: "CONFIRMED", nights: 14,
        created_at: daysAgo(1), updated_at: daysAgo(1),
        events: [{ type: "CREATED", timestamp: daysAgo(1), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000007",
        user_id: u4, hotel_id: h6,
        check_in: "2026-09-01", check_out: "2026-09-04",
        status: "CANCELLED", nights: 3,
        created_at: daysAgo(45), updated_at: daysAgo(40),
        events: [
            { type: "CREATED",   timestamp: daysAgo(45), note: "Booking created" },
            { type: "CANCELLED", timestamp: daysAgo(40), note: "Booking cancelled by user" }
        ]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000008",
        user_id: u4, hotel_id: h2,
        check_in: "2026-05-28", check_out: "2026-06-02",
        status: "CONFIRMED", nights: 5,
        created_at: daysAgo(7), updated_at: daysAgo(7),
        events: [{ type: "CREATED", timestamp: daysAgo(7), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-000000000009",
        user_id: u5, hotel_id: h3,
        check_in: "2026-06-14", check_out: "2026-06-16",
        status: "CONFIRMED", nights: 2,
        created_at: daysAgo(2), updated_at: daysAgo(2),
        events: [{ type: "CREATED", timestamp: daysAgo(2), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-00000000000a",
        user_id: u5, hotel_id: h1,
        check_in: "2026-11-01", check_out: "2026-11-05",
        status: "CONFIRMED", nights: 4,
        created_at: daysAgo(15), updated_at: daysAgo(15),
        events: [{ type: "CREATED", timestamp: daysAgo(15), note: "Booking created" }]
    },
    {
        booking_id: "b0000000-0000-0000-0000-00000000000b",
        user_id: u1, hotel_id: h6,
        check_in: "2026-10-10", check_out: "2026-10-15",
        status: "CANCELLED", nights: 5,
        created_at: daysAgo(90), updated_at: daysAgo(85),
        events: [
            { type: "CREATED",   timestamp: daysAgo(90), note: "Booking created" },
            { type: "CANCELLED", timestamp: daysAgo(85), note: "Booking cancelled by user" }
        ]
    },
    {
        booking_id: "b0000000-0000-0000-0000-00000000000c",
        user_id: u2, hotel_id: h1,
        check_in: "2027-02-14", check_out: "2027-02-16",
        status: "CONFIRMED", nights: 2,
        created_at: NumberLong(nowMs), updated_at: NumberLong(nowMs),
        events: [{ type: "CREATED", timestamp: NumberLong(nowMs), note: "Booking created" }]
    }
]);

print("[SEED]    total=" + db.bookings.countDocuments());
