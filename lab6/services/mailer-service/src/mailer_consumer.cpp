#include "mailer_consumer.hpp"

#include <chrono>

#include <userver/components/component_context.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/admin_channel.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/component.hpp>

namespace mailer {

MailerConsumer::MailerConsumer(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ConsumerComponentBase(config, context) {
    auto& rmq =
        context.FindComponent<userver::components::RabbitMQ>("booking-rabbitmq");
    auto client = rmq.GetClient();
    auto deadline =
        userver::engine::Deadline::FromDuration(std::chrono::seconds{10});
    auto admin = client->GetAdminChannel(deadline);

    admin.DeclareExchange(
        userver::urabbitmq::Exchange{"booking_events"},
        userver::urabbitmq::Exchange::Type::kTopic, {},
        userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));

    admin.DeclareQueue(
        userver::urabbitmq::Queue{"mailer_queue"}, {},
        userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));

    admin.BindQueue(
        userver::urabbitmq::Exchange{"booking_events"},
        userver::urabbitmq::Queue{"mailer_queue"}, "booking.*",
        userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));

    LOG_INFO() << "MailerConsumer: exchange, queue and binding declared";
}

void MailerConsumer::Process(std::string message) {
    LOG_INFO() << "Received booking event: " << message;
    try {
        auto json = userver::formats::json::FromString(message);
        auto event_type = json["event_type"].As<std::string>("");
        auto booking_id = json["booking_id"].As<std::string>("");
        auto user_id = json["user_id"].As<std::string>("");

        if (event_type == "booking.created") {
            auto hotel_id = json["hotel_id"].As<std::string>("");
            auto check_in = json["check_in"].As<std::string>("");
            auto check_out = json["check_out"].As<std::string>("");
            LOG_INFO() << "[MAILER] Sending confirmation email | user=" << user_id
                       << " booking=" << booking_id
                       << " hotel=" << hotel_id
                       << " dates=" << check_in << "/" << check_out;
        } else if (event_type == "booking.cancelled") {
            LOG_INFO() << "[MAILER] Sending cancellation email | user=" << user_id
                       << " booking=" << booking_id;
        } else {
            LOG_WARNING() << "[MAILER] Unknown event type: " << event_type;
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "[MAILER] Failed to process event: " << e.what();
    }
}

} // namespace mailer
