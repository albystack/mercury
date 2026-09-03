#pragma once

#include <variant>

#include <mercury/market/types.hpp>

namespace mercury::market {

// Identifies which side initiated a trade.
//
// Buy:  an aggressive buyer consumed resting ask liquidity.
// Sell: an aggressive seller consumed resting bid liquidity.
//
// This is deliberately separate from Side::Bid / Side::Ask.
// Bid/Ask describes resting book liquidity, while Buy/Sell
// describes the direction of the aggressive trade.
enum class AggressorSide { Buy, Sell };

// Represents one L2 order-book update.
//
// Quantity is the new aggregate quantity at the price level.
// A quantity of zero removes the level.
struct BookUpdate {
  Timestamp timestamp;
  Side side;
  Price price;
  Quantity quantity;
};

// Represents one executed market trade.
struct Trade {
  Timestamp timestamp;
  AggressorSide aggressor_side;
  Price price;
  Quantity quantity;
};

// A normalized market-data event consumed by Mercury.
using MarketEvent = std::variant<BookUpdate, Trade>;

} // namespace mercury::market
