#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace inventory {

struct Hotel {
  std::string id;
  std::string name;
  std::string city;
  std::string address;
  std::string description;
  int stars{0};
};

class StorageComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "storage-component";

  StorageComponent(const userver::components::ComponentConfig &config,
                   const userver::components::ComponentContext &context);

  Hotel AddHotel(const std::string &name, const std::string &city,
                 const std::string &address, const std::string &description,
                 int stars);
  std::vector<Hotel> GetHotels(const std::string &city_filter) const;
  std::optional<Hotel> FindHotel(const std::string &hotel_id) const;

private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

} // namespace inventory
