#pragma once

#include <functional>
#include <map>

#include <mercury/market/types.hpp>
#include <optional> // returns quantity, or st::nullopt
#include <stdexcept>

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

  // Apply an L2 price-level update.
  // Positive quantity: insert a new level or replace quantity at existing level
  // Zero quantity: remove price level
  // Negative quantities are invalid market state and are rejected
  void update(market::Side side, market::Price price,
              market::Quantity quantity) {
    if (quantity < 0) {
      throw std::invalid_argument{"Order book quantity cannot be negative"};
    }
    if (side == market::Side::Bid) {
      if (quantity == 0) {
        bids_.erase(price);
      } else {
        bids_.insert_or_assign(price, quantity);
      }
      return;
    }
    if (quantity == 0) {
      asks_.erase(price);
    } else {
      asks_.insert_or_assign(price, quantity);
    }
  }
  // return the aggregate quantity at a particular price. If it does not exist,
  // return std::nullopt

  [[nodiscard]]
  std::optional<market::Quantity> quantity_at(market::Side side,
                                              market::Price price) const

  {
    if (side == market::Side::Bid) {
      const auto iterator = bids_.find(price);
      if (iterator == bids_.end()) {
        return std::nullopt;
      }
      return iterator->second;
    }
    const auto iterator = asks_.find(price);
    if (iterator == asks_.end()) {
      return std::nullopt;
    }
    return iterator->second;
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
