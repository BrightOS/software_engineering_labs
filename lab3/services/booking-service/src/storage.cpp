#include "storage.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace booking {

namespace {

template <typename Row>
Booking RowToBooking(const Row &row) {
  Booking b;
  b.id = row["id"].template As<std::string>();
  b.user_id = row["user_id"].template As<std::string>();
  b.hotel_id = row["hotel_id"].template As<std::string>();
  b.check_in = row["check_in"].template As<std::string>();
  b.check_out = row["check_out"].template As<std::string>();
  b.status = row["status"].template As<std::string>();
  return b;
}

} // namespace

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context),
      pg_cluster_(
          context
              .FindComponent<userver::components::Postgres>("booking-database")
              .GetCluster()) {}

Booking StorageComponent::AddBooking(const std::string &user_id,
                                     const std::string &hotel_id,
                                     const std::string &check_in,
                                     const std::string &check_out) {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO bookings (user_id, hotel_id, check_in, check_out) "
      "VALUES ($1::uuid, $2::uuid, $3::date, $4::date) "
      "RETURNING id::text, user_id::text, hotel_id::text, "
      "to_char(check_in, 'YYYY-MM-DD') as check_in, "
      "to_char(check_out, 'YYYY-MM-DD') as check_out, status",
      user_id, hotel_id, check_in, check_out);

  return RowToBooking(result[0]);
}

std::vector<Booking>
StorageComponent::GetUserBookings(const std::string &user_id) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text, user_id::text, hotel_id::text, "
      "to_char(check_in, 'YYYY-MM-DD') as check_in, "
      "to_char(check_out, 'YYYY-MM-DD') as check_out, status "
      "FROM bookings WHERE user_id = $1::uuid "
      "ORDER BY created_at DESC",
      user_id);

  std::vector<Booking> bookings;
  for (auto row : result) {
    bookings.push_back(RowToBooking(row));
  }
  return bookings;
}

std::optional<Booking>
StorageComponent::FindBooking(const std::string &booking_id) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text, user_id::text, hotel_id::text, "
      "to_char(check_in, 'YYYY-MM-DD') as check_in, "
      "to_char(check_out, 'YYYY-MM-DD') as check_out, status "
      "FROM bookings WHERE id = $1::uuid",
      booking_id);

  if (result.IsEmpty()) {
    return std::nullopt;
  }
  return RowToBooking(result[0]);
}

bool StorageComponent::CancelBooking(const std::string &booking_id) {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "UPDATE bookings SET status = 'CANCELLED' "
      "WHERE id = $1::uuid AND status = 'CONFIRMED' "
      "RETURNING id::text",
      booking_id);
  return !result.IsEmpty();
}

} // namespace booking
