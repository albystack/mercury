#pragma once
#include <mercury/market/types.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace mercury::metrics {
struct LatencySummary {
  std::size_t count{};
  market::Timestamp min_ns{}, median_ns{}, p95_ns{}, p99_ns{}, max_ns{};
  long double mean_ns{};
};
class LatencySamples {
public:
  void add(market::Timestamp duration) { values_.push_back(duration); }
  [[nodiscard]] LatencySummary summary() const {
    if (values_.empty()) return {};
    auto sorted = values_; std::sort(sorted.begin(), sorted.end());
    long double sum = 0; for (const auto value : sorted) sum += static_cast<long double>(value);
    const auto percentile = [&](std::size_t percent) {
      const auto rank = (sorted.size() / 100) * percent + ((sorted.size() % 100) * percent + 99) / 100;
      return sorted[rank - 1];
    };
    return {sorted.size(), sorted.front(), percentile(50), percentile(95), percentile(99), sorted.back(),
            sum / static_cast<long double>(sorted.size())};
  }
private:
  std::vector<market::Timestamp> values_;
};
} // namespace mercury::metrics
