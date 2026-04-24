db = db.getSiblingDB("booking_db");

db.createCollection("bookings", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["booking_id", "user_id", "hotel_id", "check_in", "check_out", "status", "created_at"],
      additionalProperties: true,
      properties: {
        booking_id: {
          bsonType: "string",
          description: "UUID of the booking — required, must be a string",
          pattern: "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
        },
        user_id: {
          bsonType: "string",
          description: "UUID of the user — required, must be a string",
          pattern: "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
        },
        hotel_id: {
          bsonType: "string",
          description: "UUID of the hotel — required, must be a string",
          pattern: "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
        },
        check_in: {
          bsonType: "string",
          description: "Check-in date in YYYY-MM-DD format",
          pattern: "^\\d{4}-\\d{2}-\\d{2}$"
        },
        check_out: {
          bsonType: "string",
          description: "Check-out date in YYYY-MM-DD format",
          pattern: "^\\d{4}-\\d{2}-\\d{2}$"
        },
        status: {
          bsonType: "string",
          enum: ["CONFIRMED", "CANCELLED"],
          description: "Booking status — must be CONFIRMED or CANCELLED"
        },
        created_at: {
          bsonType: "long",
          description: "Creation timestamp (ms since epoch) — required"
        },
        updated_at: {
          bsonType: "long",
          description: "Last update timestamp (ms since epoch)"
        },
        events: {
          bsonType: "array",
          description: "Embedded history of booking lifecycle events",
          items: {
            bsonType: "object",
            required: ["type", "timestamp"],
            properties: {
              type: {
                bsonType: "string",
                enum: ["CREATED", "CANCELLED"],
                description: "Event type"
              },
              timestamp: {
                bsonType: "long",
                description: "When the event occurred (ms since epoch)"
              },
              note: {
                bsonType: "string",
                description: "Human-readable description of the event"
              }
            }
          }
        }
      }
    }
  },
  validationAction: "warn",
  validationLevel: "strict"
});

// Unique index on booking_id
db.bookings.createIndex({ booking_id: 1 }, { unique: true });

// Index for user booking lookups (most common query pattern)
db.bookings.createIndex({ user_id: 1, created_at: -1 });

// Index for hotel analytics
db.bookings.createIndex({ hotel_id: 1, status: 1 });

// Index for status-based queries
db.bookings.createIndex({ status: 1, created_at: -1 });

print("[INIT]    bookings collection created with $jsonSchema validator and indexes");
