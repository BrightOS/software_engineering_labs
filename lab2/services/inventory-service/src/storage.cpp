#include "storage.hpp"

#include <userver/utils/uuid4.hpp>

namespace inventory {

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

Hotel StorageComponent::AddHotel(const std::string &name,
                                 const std::string &city,
                                 const std::string &address,
                                 const std::string &description, int stars) {
  std::lock_guard lock(mutex_);
  Hotel hotel;
  hotel.id = userver::utils::generators::GenerateUuid();
  hotel.name = name;
  hotel.city = city;
  hotel.address = address;
  hotel.description = description;
  hotel.stars = stars;
  hotels_[hotel.id] = hotel;
  return hotel;
}

std::vector<Hotel>
StorageComponent::GetHotels(const std::string &city_filter) const {
  std::lock_guard lock(mutex_);
  std::vector<Hotel> result;
  for (const auto &[id, hotel] : hotels_) {
    if (city_filter.empty() || hotel.city == city_filter) {
      result.push_back(hotel);
    }
  }
  return result;
}

std::optional<Hotel>
StorageComponent::FindHotel(const std::string &hotel_id) const {
  std::lock_guard lock(mutex_);
  auto it = hotels_.find(hotel_id);
  if (it == hotels_.end())
    return std::nullopt;
  return it->second;
}

} // namespace inventory
