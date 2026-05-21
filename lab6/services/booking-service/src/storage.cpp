#include "storage.hpp"

#include <chrono>

#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/options.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace booking {

namespace {

namespace bson = userver::formats::bson;
namespace mongo_ops = userver::storages::mongo::operations;
namespace mongo_opts = userver::storages::mongo::options;

int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

Booking DocToBooking(const bson::Value& doc) {
    Booking b;
    b.id       = doc["booking_id"].As<std::string>();
    b.user_id  = doc["user_id"].As<std::string>();
    b.hotel_id = doc["hotel_id"].As<std::string>();
    b.check_in  = doc["check_in"].As<std::string>();
    b.check_out = doc["check_out"].As<std::string>();
    b.status   = doc["status"].As<std::string>();
    return b;
}

} // namespace

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      mongo_pool_(
          context.FindComponent<userver::components::Mongo>("booking-mongo").GetPool()) {}

Booking StorageComponent::AddBooking(const std::string& user_id,
                                     const std::string& hotel_id,
                                     const std::string& check_in,
                                     const std::string& check_out) {
    static thread_local boost::uuids::random_generator uuid_gen;
    auto booking_id = boost::uuids::to_string(uuid_gen());
    auto now = NowMs();

    bson::ValueBuilder created_event;
    created_event["type"] = std::string("CREATED");
    created_event["timestamp"] = now;
    created_event["note"] = std::string("Booking created");

    bson::ValueBuilder events_arr(bson::ValueBuilder::Type::kArray);
    events_arr.PushBack(created_event.ExtractValue());

    std::string status_confirmed = "CONFIRMED";

    bson::ValueBuilder doc;
    doc["booking_id"] = booking_id;
    doc["user_id"]    = user_id;
    doc["hotel_id"]   = hotel_id;
    doc["check_in"]   = check_in;
    doc["check_out"]  = check_out;
    doc["status"]     = status_confirmed;
    doc["created_at"] = now;
    doc["updated_at"] = now;
    doc["events"]     = events_arr.ExtractValue();

    auto coll = mongo_pool_->GetCollection("bookings");
    coll.InsertOne(doc.ExtractValue());

    Booking b;
    b.id = booking_id;
    b.user_id = user_id;
    b.hotel_id = hotel_id;
    b.check_in = check_in;
    b.check_out = check_out;
    b.status = status_confirmed;
    return b;
}

std::vector<Booking> StorageComponent::GetUserBookings(const std::string& user_id) const {
    auto coll = mongo_pool_->GetCollection("bookings");

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;

    mongo_ops::Find op(filter.ExtractValue());
    op.SetOption(mongo_opts::Sort{{"created_at", mongo_opts::Sort::kDescending}});

    std::vector<Booking> result;
    for (auto& doc : coll.Execute(op)) {
        result.push_back(DocToBooking(doc));
    }
    return result;
}

std::optional<Booking> StorageComponent::FindBooking(const std::string& booking_id) const {
    auto coll = mongo_pool_->GetCollection("bookings");

    bson::ValueBuilder filter;
    filter["booking_id"] = booking_id;

    auto doc = coll.FindOne(filter.ExtractValue());
    if (!doc) {
        return std::nullopt;
    }
    return DocToBooking(*doc);
}

bool StorageComponent::CancelBooking(const std::string& booking_id) {
    auto coll = mongo_pool_->GetCollection("bookings");
    auto now = NowMs();

    bson::ValueBuilder cancel_event;
    cancel_event["type"] = std::string("CANCELLED");
    cancel_event["timestamp"] = now;
    cancel_event["note"] = std::string("Booking cancelled by user");

    bson::ValueBuilder filter;
    filter["booking_id"] = booking_id;
    filter["status"] = std::string("CONFIRMED");

    std::string status_cancelled = "CANCELLED";

    bson::ValueBuilder set_fields;
    set_fields["status"] = status_cancelled;
    set_fields["updated_at"] = now;

    bson::ValueBuilder push_fields;
    push_fields["events"] = cancel_event.ExtractValue();

    bson::ValueBuilder update;
    update["$set"] = set_fields.ExtractValue();
    update["$push"] = push_fields.ExtractValue();

    auto write_result = coll.UpdateOne(filter.ExtractValue(), update.ExtractValue());

    return write_result.MatchedCount() > 0;
}

} // namespace booking
