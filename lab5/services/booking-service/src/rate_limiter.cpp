#include "rate_limiter.hpp"

#include <algorithm>

namespace booking {

RateLimiterComponent::RateLimiterComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      statistics_holder_(
          context.FindComponent<userver::components::StatisticsStorage>()
              .GetStorage()
              .RegisterWriter(
                  "booking-rate-limiter",
                  [this](userver::utils::statistics::Writer& writer) {
                      WriteMetrics(writer);
                  })) {}

RateLimitResult RateLimiterComponent::CheckLimit(const std::string& user_id) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(mutex_);

    auto it = buckets_.find(user_id);
    if (it == buckets_.end()) {
        buckets_[user_id] = {static_cast<double>(kMaxTokens) - 1.0, now};
        ++requests_allowed_;
        int remaining = kMaxTokens - 1;
        long long reset = static_cast<long long>(1.0 / kRefillPerSecond);
        return {true, kMaxTokens, remaining, reset};
    }

    auto& bucket = it->second;
    double elapsed =
        std::chrono::duration<double>(now - bucket.last_refill).count();
    bucket.tokens = std::min(static_cast<double>(kMaxTokens),
                             bucket.tokens + elapsed * kRefillPerSecond);
    bucket.last_refill = now;

    if (bucket.tokens >= 1.0) {
        bucket.tokens -= 1.0;
        int remaining = static_cast<int>(bucket.tokens);
        long long reset = static_cast<long long>(
            (static_cast<double>(kMaxTokens) - bucket.tokens) /
            kRefillPerSecond);
        ++requests_allowed_;
        return {true, kMaxTokens, remaining, reset};
    }

    double tokens_needed = 1.0 - bucket.tokens;
    long long reset =
        static_cast<long long>(tokens_needed / kRefillPerSecond) + 1;
    ++requests_rejected_;
    return {false, kMaxTokens, 0, reset};
}

void RateLimiterComponent::WriteMetrics(
    userver::utils::statistics::Writer& writer) const {
    writer["allowed"] = requests_allowed_.load();
    writer["rejected"] = requests_rejected_.load();
}

} // namespace booking
