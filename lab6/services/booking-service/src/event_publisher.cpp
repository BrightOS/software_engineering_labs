#include "event_publisher.hpp"

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/component.hpp>

namespace booking {

namespace {
constexpr std::string_view kExchangeName = "booking_events";
}

EventPublisherComponent::EventPublisherComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      client_(context.FindComponent<userver::components::RabbitMQ>("booking-rabbitmq")
                  .GetClient()) {
    try {
        auto deadline =
            userver::engine::Deadline::FromDuration(std::chrono::seconds{10});
        auto admin = client_->GetAdminChannel(deadline);
        admin.DeclareExchange(
            userver::urabbitmq::Exchange{std::string(kExchangeName)},
            userver::urabbitmq::Exchange::Type::kTopic, {},
            userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
        LOG_INFO() << "RabbitMQ exchange '" << kExchangeName << "' declared";
    } catch (const std::exception& e) {
        LOG_WARNING() << "RabbitMQ exchange declaration failed: " << e.what()
                      << " — will retry on first publish";
    }
}

void EventPublisherComponent::PublishBookingCreated(const Booking& b) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = std::string("booking.created");
    payload["booking_id"] = b.id;
    payload["user_id"] = b.user_id;
    payload["hotel_id"] = b.hotel_id;
    payload["check_in"] = b.check_in;
    payload["check_out"] = b.check_out;
    payload["status"] = b.status;

    try {
        client_->Publish(
            userver::urabbitmq::Exchange{std::string(kExchangeName)},
            "booking.created",
            userver::formats::json::ToString(payload.ExtractValue()),
            userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
        LOG_INFO() << "Published booking.created event for booking " << b.id;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to publish booking.created: " << e.what();
    }
}

void EventPublisherComponent::PublishBookingCancelled(
    const std::string& booking_id, const std::string& user_id) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = std::string("booking.cancelled");
    payload["booking_id"] = booking_id;
    payload["user_id"] = user_id;

    try {
        client_->Publish(
            userver::urabbitmq::Exchange{std::string(kExchangeName)},
            "booking.cancelled",
            userver::formats::json::ToString(payload.ExtractValue()),
            userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
        LOG_INFO() << "Published booking.cancelled event for booking "
                   << booking_id;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to publish booking.cancelled: " << e.what();
    }
}

} // namespace booking
