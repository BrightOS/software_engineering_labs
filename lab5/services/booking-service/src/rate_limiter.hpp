#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

namespace booking {

struct RateLimitResult {
    bool allowed;
    int limit;
    int remaining;
    long long reset_seconds;
};

class RateLimiterComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rate-limiter";

    RateLimiterComponent(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

    RateLimitResult CheckLimit(const std::string& user_id);

private:
    void WriteMetrics(userver::utils::statistics::Writer& writer) const;

    static constexpr int kMaxTokens = 10;
    static constexpr double kRefillPerSecond = 10.0 / 60.0;

    struct Bucket {
        double tokens;
        std::chrono::steady_clock::time_point last_refill;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Bucket> buckets_;

    mutable std::atomic<uint64_t> requests_allowed_{0};
    mutable std::atomic<uint64_t> requests_rejected_{0};

    userver::utils::statistics::Entry statistics_holder_;
};

} // namespace booking
