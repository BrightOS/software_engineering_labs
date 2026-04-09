#include "booking_handler.hpp"
#include "storage.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char *argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<booking::StorageComponent>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::Postgres>("booking-database")
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::HttpClient>()
          .Append<userver::server::handlers::Ping>();

  booking::AppendBookingHandlers(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
