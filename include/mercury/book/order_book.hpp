#pragma once

#include <functional>
#include <map>

#include <mercury/market/types.hpp>

namespace mercury::book {

// A single aggregated L2 Price Level
//
// Example:
//  price = 10'000 ticks
//  quantity = 500 units
//  means the book currently contains 500 aggregate quantity units resting at
//  that price we will use prive level later when returning the best bid or
//  ask
// namespace mercury::book
struct PriceLevel {
  market::Price price;
  market::Quantity quantity;
};

// Stores mercury's current local view of an L2 limit order book
class OrderBook {
public:
  // A newly created order book contains no bids, no asks
  OrderBook() = default;
  // Return true when the requested side of the book contains no price levels
  // Example:
  // book.empty(market::Side::Bid)
  // returns true if there are currently no bids
  [[nodiscard]]
  bool empty(market::Side side) const noexcept {
    if (side == market::Side::Bid) {
      return bids_.empty();
    }
    return asks_.empty();
  }

private:
  // Internal storage
  // Bids must be ordered from HIGHEST TO LOWEST
  // std::greater<market::Price> reverses std::map's normal ascending order
  using BidLevels =
      std::map<market::Price, market::Quantity, std::greater<market::Price>>;

  // Asks use std:map's normal ascending order
  using AskLevels = std::map<market::Price, market::Quantity>;

  BidLevels bids_;
  AskLevels asks_;
};
} // namespace mercury::book
