#pragma once

#include <variant>

#include <mercury/book/order_book.hpp>
#include <mercury/market/events.hpp>

namespace mercury::market {

class MarketDataProcessor {
public:
  explicit MarketDataProcessor(mercury::book::OrderBook &book) noexcept
      : book_{book} {}

  void process(const MarketEvent &event) {
    std::visit([this](const auto &value) { handle(value); }, event);
  }

private:
  void handle(const BookUpdate &update) {
    book_.update(update.side, update.price, update.quantity);
  }

  void handle(const Trade &) noexcept {
    // Trades do not directly mutate authoritative L2 book state.
  }

  mercury::book::OrderBook &book_;
};

} // namespace mercury::market
