#include "mailer_consumer.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/urabbitmq/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::clients::dns::Component>()
            .Append<userver::components::Secdist>()
            .Append<userver::components::DefaultSecdistProvider>()
            .Append<userver::components::RabbitMQ>("booking-rabbitmq")
            .Append<mailer::MailerConsumer>()
            .Append<userver::server::handlers::Ping>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
