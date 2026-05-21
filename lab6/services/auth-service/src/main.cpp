#include "auth_handler.hpp"
#include "storage.hpp"
#include "user_handler.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<auth::StorageComponent>()
            .Append<userver::components::TestsuiteSupport>()
            .Append<userver::components::Postgres>("auth-database")
            .Append<userver::clients::dns::Component>()
            .Append<userver::server::handlers::ServerMonitor>()
            .Append<userver::server::handlers::Ping>();

    auth::AppendAuthHandlers(component_list);
    auth::AppendUserHandlers(component_list);

    return userver::utils::DaemonMain(argc, argv, component_list);
}
