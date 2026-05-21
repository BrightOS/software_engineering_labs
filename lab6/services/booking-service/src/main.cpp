#include "booking_handler.hpp"
#include "event_publisher.hpp"
#include "rate_limiter.hpp"
#include "storage.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<booking::StorageComponent>()
            .Append<booking::RateLimiterComponent>()
            .Append<booking::EventPublisherComponent>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::Mongo>("booking-mongo")
            .Append<userver::clients::dns::Component>()
            .Append<userver::components::HttpClient>()
            .Append<userver::components::Secdist>()
            .Append<userver::components::DefaultSecdistProvider>()
            .Append<userver::components::RabbitMQ>("booking-rabbitmq")
            .Append<userver::server::handlers::ServerMonitor>()
            .Append<userver::server::handlers::Ping>();

    booking::AppendBookingHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
