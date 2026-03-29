#include "storage.hpp"

#include <userver/utils/uuid4.hpp>

namespace booking {

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

Booking StorageComponent::AddBooking(const std::string &user_id,
                                     const std::string &hotel_id,
                                     const std::string &check_in,
                                     const std::string &check_out) {
  std::lock_guard lock(mutex_);
  Booking booking;
  booking.id = userver::utils::generators::GenerateUuid();
  booking.user_id = user_id;
  booking.hotel_id = hotel_id;
  booking.check_in = check_in;
  booking.check_out = check_out;
  booking.status = "CONFIRMED";
  bookings_[booking.id] = booking;
  return booking;
}

std::vector<Booking>
StorageComponent::GetUserBookings(const std::string &user_id) const {
  std::lock_guard lock(mutex_);
  std::vector<Booking> result;
  for (const auto &[id, booking] : bookings_) {
    if (booking.user_id == user_id) {
      result.push_back(booking);
    }
  }
  return result;
}

std::optional<Booking>
StorageComponent::FindBooking(const std::string &booking_id) const {
  std::lock_guard lock(mutex_);
  auto it = bookings_.find(booking_id);
  if (it == bookings_.end())
    return std::nullopt;
  return it->second;
}

bool StorageComponent::CancelBooking(const std::string &booking_id) {
  std::lock_guard lock(mutex_);
  auto it = bookings_.find(booking_id);
  if (it == bookings_.end())
    return false;
  if (it->second.status == "CANCELLED")
    return false;
  it->second.status = "CANCELLED";
  return true;
}

} // namespace booking
