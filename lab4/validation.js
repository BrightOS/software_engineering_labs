db = db.getSiblingDB("booking_db");

db.runCommand({ collMod: "bookings", validationAction: "error" });

print("[TEST 1]  valid document -- should pass");
try {
    db.bookings.insertOne({
        booking_id: "ffffffff-0000-0000-0000-000000000001",
        user_id: "10000000-0000-0000-0000-000000000001",
        hotel_id: "20000000-0000-0000-0000-000000000001",
        check_in: "2026-09-01",
        check_out: "2026-09-05",
        status: "CONFIRMED",
        created_at: NumberLong(new Date().getTime()),
        events: [{ type: "CREATED", timestamp: NumberLong(new Date().getTime()), note: "ok" }]
    });
    print("          [PASS] inserted");
} catch(e) {
    print("          [FAIL] " + e.message);
}

print("[TEST 2]  missing booking_id -- should fail");
try {
    db.bookings.insertOne({
        user_id: "10000000-0000-0000-0000-000000000001",
        hotel_id: "20000000-0000-0000-0000-000000000001",
        check_in: "2026-09-01",
        check_out: "2026-09-05",
        status: "CONFIRMED",
        created_at: NumberLong(new Date().getTime())
    });
    print("          [FAIL] inserted but should have been rejected");
} catch(e) {
    print("          [PASS] rejected as expected");
}

print("[TEST 3]  status=PENDING -- should fail");
try {
    db.bookings.insertOne({
        booking_id: "ffffffff-0000-0000-0000-000000000002",
        user_id: "10000000-0000-0000-0000-000000000001",
        hotel_id: "20000000-0000-0000-0000-000000000001",
        check_in: "2026-09-01",
        check_out: "2026-09-05",
        status: "PENDING",
        created_at: NumberLong(new Date().getTime())
    });
    print("          [FAIL] inserted but should have been rejected");
} catch(e) {
    print("          [PASS] rejected as expected");
}

print("[TEST 4]  booking_id not uuid format -- should fail");
try {
    db.bookings.insertOne({
        booking_id: "not-a-uuid",
        user_id: "10000000-0000-0000-0000-000000000001",
        hotel_id: "20000000-0000-0000-0000-000000000001",
        check_in: "2026-09-01",
        check_out: "2026-09-05",
        status: "CONFIRMED",
        created_at: NumberLong(new Date().getTime())
    });
    print("          [FAIL] inserted but should have been rejected");
} catch(e) {
    print("          [PASS] rejected as expected");
}

print("[TEST 5]  created_at=string instead of long -- should fail");
try {
    db.bookings.insertOne({
        booking_id: "ffffffff-0000-0000-0000-000000000003",
        user_id: "10000000-0000-0000-0000-000000000001",
        hotel_id: "20000000-0000-0000-0000-000000000001",
        check_in: "2026-09-01",
        check_out: "2026-09-05",
        status: "CONFIRMED",
        created_at: "not-a-number"
    });
    print("          [FAIL] inserted but should have been rejected");
} catch(e) {
    print("          [PASS] rejected as expected");
}

db.bookings.deleteOne({ booking_id: "ffffffff-0000-0000-0000-000000000001" });
db.runCommand({ collMod: "bookings", validationAction: "warn" });
print("[DONE]    test data cleaned up, validationAction restored to warn");
