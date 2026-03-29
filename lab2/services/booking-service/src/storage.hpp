#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace booking {

struct Booking {
  std::string id;
  std::string user_id;
  std::string hotel_id;
  std::string check_in;
  std::string check_out;
  std::string status;
};

class StorageComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "storage-component";

  StorageComponent(const userver::components::ComponentConfig &config,
                   const userver::components::ComponentContext &context);

  Booking AddBooking(const std::string &user_id, const std::string &hotel_id,
                     const std::string &check_in, const std::string &check_out);
  std::vector<Booking> GetUserBookings(const std::string &user_id) const;
  std::optional<Booking> FindBooking(const std::string &booking_id) const;
  bool CancelBooking(const std::string &booking_id);

private:
  mutable std::mutex mutex_;
  std::map<std::string, Booking> bookings_;
};

} // namespace booking
