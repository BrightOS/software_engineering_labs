#pragma once

#include <memory>
#include <string>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/urabbitmq/client.hpp>

#include "storage.hpp"

namespace booking {

class EventPublisherComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "event-publisher";

    EventPublisherComponent(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context);

    void PublishBookingCreated(const Booking& b) const;
    void PublishBookingCancelled(const std::string& booking_id,
                                  const std::string& user_id) const;

private:
    std::shared_ptr<userver::urabbitmq::Client> client_;
};

} // namespace booking
