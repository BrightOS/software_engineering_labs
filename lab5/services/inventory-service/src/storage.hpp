#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

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

    StorageComponent(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

    Hotel AddHotel(const std::string& name, const std::string& city,
                   const std::string& address, const std::string& description,
                   int stars);
    std::vector<Hotel> GetHotels(const std::string& city_filter) const;
    std::optional<Hotel> FindHotel(const std::string& hotel_id) const;

private:
    void WriteMetrics(userver::utils::statistics::Writer& writer) const;

    static constexpr auto kListCacheTtl = std::chrono::minutes{5};
    static constexpr auto kHotelCacheTtl = std::chrono::minutes{10};

    struct ListCacheEntry {
        std::vector<Hotel> hotels;
        std::chrono::steady_clock::time_point expires_at;
    };

    struct HotelCacheEntry {
        Hotel hotel;
        std::chrono::steady_clock::time_point expires_at;
    };

    userver::storages::postgres::ClusterPtr pg_cluster_;

    mutable std::mutex cache_mutex_;
    mutable std::unordered_map<std::string, ListCacheEntry> list_cache_;
    mutable std::unordered_map<std::string, HotelCacheEntry> hotel_cache_;

    mutable std::atomic<uint64_t> list_cache_hits_{0};
    mutable std::atomic<uint64_t> list_cache_misses_{0};
    mutable std::atomic<uint64_t> hotel_cache_hits_{0};
    mutable std::atomic<uint64_t> hotel_cache_misses_{0};
    userver::utils::statistics::Entry statistics_holder_;
};

} // namespace inventory
