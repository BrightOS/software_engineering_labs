#include "storage.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace inventory {

namespace {

template <typename Row>
Hotel RowToHotel(const Row &row) {
  Hotel h;
  h.id = row["id"].template As<std::string>();
  h.name = row["name"].template As<std::string>();
  h.city = row["city"].template As<std::string>();
  h.address = row["address"].template As<std::string>();
  h.description = row["description"].template As<std::string>();
  h.stars = row["stars"].template As<int>();
  return h;
}

} // namespace

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context),
      pg_cluster_(
          context
              .FindComponent<userver::components::Postgres>(
                  "inventory-database")
              .GetCluster()) {}

Hotel StorageComponent::AddHotel(const std::string &name,
                                 const std::string &city,
                                 const std::string &address,
                                 const std::string &description, int stars) {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO hotels (name, city, address, description, stars) "
      "VALUES ($1, $2, $3, $4, $5) "
      "RETURNING id::text, name, city, address, "
      "COALESCE(description, '') as description, stars",
      name, city, address, description, stars);

  return RowToHotel(result[0]);
}

std::vector<Hotel>
StorageComponent::GetHotels(const std::string &city_filter) const {
  auto result = city_filter.empty()
      ? pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id::text, name, city, address, "
            "COALESCE(description, '') as description, stars "
            "FROM hotels ORDER BY created_at DESC")
      : pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id::text, name, city, address, "
            "COALESCE(description, '') as description, stars "
            "FROM hotels WHERE city = $1 ORDER BY created_at DESC",
            city_filter);

  std::vector<Hotel> hotels;
  for (auto row : result) {
    hotels.push_back(RowToHotel(row));
  }
  return hotels;
}

std::optional<Hotel>
StorageComponent::FindHotel(const std::string &hotel_id) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text, name, city, address, "
      "COALESCE(description, '') as description, stars "
      "FROM hotels WHERE id = $1::uuid",
      hotel_id);

  if (result.IsEmpty()) {
    return std::nullopt;
  }
  return RowToHotel(result[0]);
}

} // namespace inventory
