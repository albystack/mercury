#pragma once
#include <mercury/book/order_book.hpp>
#include <algorithm>
#include <limits>
namespace mercury::strategy {
struct Quote {
  market::Price price;
  market::Quantity quantity;
};
struct Quotes {
  std::optional<Quote> bid;
  std::optional<Quote> ask;
};
struct QuoteConfig {
  market::Quantity quantity{100'000};
  std::int64_t half_spread_ticks{1};
  std::int64_t max_skew_ticks{10};
  market::Quantity inventory_limit{1'000'000};
};
class InventoryQuoter {
public:
  explicit InventoryQuoter(QuoteConfig config = {}) : config_{config} {
    if (config.quantity <= 0 || config.half_spread_ticks <= 0 || config.max_skew_ticks < 0 ||
        config.inventory_limit < config.quantity ||
        config.half_spread_ticks > std::numeric_limits<std::int64_t>::max() - config.max_skew_ticks)
      throw std::invalid_argument{"Invalid quoting configuration"};
  }
  [[nodiscard]] Quotes quotes(const book::OrderBook& book, market::Quantity inventory) const {
    const auto bid = book.best_bid(), ask = book.best_ask();
    if (!bid || !ask || bid->price.ticks() <= 0 || bid->price >= ask->price) return {};
    const auto midpoint = bid->price.ticks() + (ask->price.ticks() - bid->price.ticks()) / 2;
    const auto margin = config_.half_spread_ticks + config_.max_skew_ticks;
    if (midpoint <= margin || midpoint > std::numeric_limits<std::int64_t>::max() - margin) return {};
    const auto skew = std::clamp(inventory / config_.quantity, -config_.max_skew_ticks, config_.max_skew_ticks);
    const auto center = midpoint - skew;
    const auto bid_price = std::min(center - config_.half_spread_ticks, ask->price.ticks() - 1);
    const auto ask_price = std::max(center + config_.half_spread_ticks, bid->price.ticks() + 1);
    Quotes result;
    if (inventory < config_.inventory_limit) result.bid = Quote{market::Price{bid_price}, config_.quantity};
    if (inventory > -config_.inventory_limit) result.ask = Quote{market::Price{ask_price}, config_.quantity};
    return result;
  }
private:
  QuoteConfig config_;
};
} // namespace mercury::strategy
