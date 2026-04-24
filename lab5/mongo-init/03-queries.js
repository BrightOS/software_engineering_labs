db = db.getSiblingDB("booking_db");

print("[CHECK]   total="     + db.bookings.countDocuments());
print("[CHECK]   confirmed=" + db.bookings.countDocuments({ status: "CONFIRMED" }));
print("[CHECK]   cancelled=" + db.bookings.countDocuments({ status: "CANCELLED" }));
