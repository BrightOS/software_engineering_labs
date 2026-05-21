#pragma once

#include <userver/urabbitmq/consumer_component_base.hpp>

namespace mailer {

class MailerConsumer final
    : public userver::urabbitmq::ConsumerComponentBase {
public:
    static constexpr std::string_view kName = "mailer-consumer";

    MailerConsumer(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

    void Process(std::string message) override;
};

} // namespace mailer
