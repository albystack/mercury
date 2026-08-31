#pragma once

#include <mercury/market/types.hpp>

namespace mercury::market {

// Identifies which side initiated a trade.
// Buy: an aggressive buyer consumed resting ask liqudiity
// Sell: an aggressive seller consumed resting bid liquidity
// This is deliberatrly seperate from Side::Bid / Side::Ask
// "Bid/Ask" describes where liquidity rests in the order book
// "Buy/Sell" describes the direction of the aggressive trade
enum class AggressorSide { Buy, Sell };

// Represent one L2 order book update
// An exchange style L2 update tells us the new aggregate quantity resting at
// one price level
//
// A quantity of zero means the price level should be removed
struct BookUpdate {
  Timestamp timestamp;
  Side side;
  Price price;
  Quantity quantity;
};

// Represents one executed market trade
//
// A trade records:
//
// - when the transaction occurred
// - which direction initiated it
// - the execution price
// - the executed quantity
//
// Example:
//
//     timestamp       = 2'000'000
//     aggressor_side  = AggressorSide::Buy
//     price           = Price{10'001}
//     quantity        = 100
//
// means:
//     "At this timestamp, an aggressive buyer purchased
//      100 units at 10,001 ticks."
struct Trade {
  Timestamp timestamp;
  AggressorSide aggressor_side;
  Price price;
  Quantity quantity;
};

} // namespace mercury::market
