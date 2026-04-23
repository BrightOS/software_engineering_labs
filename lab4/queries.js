db = db.getSiblingDB("booking_db");

// Create

db.bookings.insertOne({
    booking_id: "eeeeeeee-0000-0000-0000-000000000001",
    user_id: "10000000-0000-0000-0000-000000000003",
    hotel_id: "20000000-0000-0000-0000-000000000002",
    check_in: "2026-10-01",
    check_out: "2026-10-04",
    status: "CONFIRMED",
    nights: 3,
    created_at: NumberLong(new Date().getTime()),
    updated_at: NumberLong(new Date().getTime()),
    events: [
        { type: "CREATED", timestamp: NumberLong(new Date().getTime()), note: "test booking" }
    ]
});
print("[INSERT]  insertOne: OK");

db.bookings.insertMany([
    {
        booking_id: "eeeeeeee-0000-0000-0000-000000000002",
        user_id: "10000000-0000-0000-0000-000000000004",
        hotel_id: "20000000-0000-0000-0000-000000000004",
        check_in: "2026-11-10",
        check_out: "2026-11-14",
        status: "CONFIRMED",
        nights: 4,
        created_at: NumberLong(new Date().getTime()),
        updated_at: NumberLong(new Date().getTime()),
        events: [{ type: "CREATED", timestamp: NumberLong(new Date().getTime()), note: "batch" }]
    },
    {
        booking_id: "eeeeeeee-0000-0000-0000-000000000003",
        user_id: "10000000-0000-0000-0000-000000000005",
        hotel_id: "20000000-0000-0000-0000-000000000003",
        check_in: "2026-12-01",
        check_out: "2026-12-03",
        status: "CONFIRMED",
        nights: 2,
        created_at: NumberLong(new Date().getTime()),
        updated_at: NumberLong(new Date().getTime()),
        events: [{ type: "CREATED", timestamp: NumberLong(new Date().getTime()), note: "batch" }]
    }
]);
print("[INSERT]  insertMany: total=" + db.bookings.countDocuments());


// Read

// findOne by exact id ($eq)
var b = db.bookings.findOne({ booking_id: { $eq: "b0000000-0000-0000-0000-000000000001" } });
print("[READ]    $eq: id=" + b.booking_id + " status=" + b.status);

print("[READ]    find+sort by user 001:");
db.bookings.find(
    { user_id: "10000000-0000-0000-0000-000000000001" }
).sort({ created_at: -1 }).forEach(function(doc) {
    print("            " + doc.booking_id + " status=" + doc.status + " check_in=" + doc.check_in);
});

var res = db.bookings.countDocuments({
    status: "CONFIRMED",
    check_in: { $gt: "2026-07-01" }
});
print("[READ]    $gt check_in>2026-07-01: count=" + res);

// $gte + $lte
print("[READ]    $gte/$lte nights 3-7: count=" + db.bookings.countDocuments({
    nights: { $gte: 3, $lte: 7 }
}));

// $ne
print("[READ]    $ne status!=CANCELLED: count=" + db.bookings.countDocuments({ status: { $ne: "CANCELLED" } }));

// $in
print("[READ]    $in hotel Grand Palace or Sochi: count=" + db.bookings.countDocuments({
    hotel_id: { $in: [
        "20000000-0000-0000-0000-000000000001",
        "20000000-0000-0000-0000-000000000004"
    ]}
}));

print("[READ]    $and CONFIRMED+nights>4: count=" + db.bookings.countDocuments({
    $and: [
        { status: "CONFIRMED" },
        { nights: { $gt: 4 } }
    ]
}));

print("[READ]    $or user 001 or 002: count=" + db.bookings.countDocuments({
    $or: [
        { user_id: "10000000-0000-0000-0000-000000000001" },
        { user_id: "10000000-0000-0000-0000-000000000002" }
    ]
}));

// $size
print("[READ]    $size events==2: count=" + db.bookings.countDocuments({
    events: { $size: 2 }
}));

// $elemMatch
print("[READ]    $elemMatch events.type=CANCELLED: count=" + db.bookings.countDocuments({
    events: { $elemMatch: { type: "CANCELLED" } }
}));


// Update

// $set + $push
var upd = db.bookings.updateOne(
    { booking_id: "eeeeeeee-0000-0000-0000-000000000001", status: "CONFIRMED" },
    {
        $set: { status: "CANCELLED", updated_at: NumberLong(new Date().getTime()) },
        $push: { events: { type: "CANCELLED", timestamp: NumberLong(new Date().getTime()), note: "cancelled during test" } }
    }
);
print("[UPDATE]  $set+$push: matched=" + upd.matchedCount + " modified=" + upd.modifiedCount);

// $exists
var filled = db.bookings.updateMany(
    { nights: { $exists: false } },
    { $set: { nights: 0 } }
);
print("[UPDATE]  $exists+$set nights: modified=" + filled.modifiedCount);

// $inc
db.bookings.updateOne(
    { booking_id: "b0000000-0000-0000-0000-000000000005" },
    { $inc: { nights: 1 } }
);
print("[UPDATE]  $inc nights: OK");

// $addToSet — add a CREATED event without duplicates (idempotent)
var addedEvent = { type: "CREATED", timestamp: NumberLong(new Date().getTime()), note: "re-confirmed" };
db.bookings.updateOne(
    { booking_id: "b0000000-0000-0000-0000-000000000005" },
    { $addToSet: { events: addedEvent } }
);
print("[UPDATE]  $addToSet events: OK");

// $pull — remove that same event
db.bookings.updateOne(
    { booking_id: "b0000000-0000-0000-0000-000000000005" },
    { $pull: { events: { note: "re-confirmed" } } }
);
print("[UPDATE]  $pull events: OK");


// Delete

var d1 = db.bookings.deleteOne({ booking_id: "eeeeeeee-0000-0000-0000-000000000002" });
print("[DELETE]  deleteOne: removed=" + d1.deletedCount);

var d2 = db.bookings.deleteMany({
    booking_id: { $in: [
        "eeeeeeee-0000-0000-0000-000000000001",
        "eeeeeeee-0000-0000-0000-000000000003"
    ]}
});
print("[DELETE]  deleteMany: removed=" + d2.deletedCount);


// Aggregation

print("[AGG]     confirmed bookings grouped by hotel (min 2 bookings):");
db.bookings.aggregate([
    { $match: { status: "CONFIRMED" } },
    {
        $group: {
            _id: "$hotel_id",
            total_bookings: { $sum: 1 },
            total_nights: { $sum: "$nights" },
            avg_nights: { $avg: "$nights" }
        }
    },
    { $match: { total_bookings: { $gte: 2 } } },
    { $sort: { total_nights: -1 } },
    {
        $project: {
            _id: 0,
            hotel_id: "$_id",
            total_bookings: 1,
            total_nights: 1,
            avg_nights: { $round: ["$avg_nights", 1] }
        }
    }
]).forEach(function(row) {
    printjson(row);
});

print("[DONE]    total documents in collection: " + db.bookings.countDocuments());
