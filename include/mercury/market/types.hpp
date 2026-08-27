/// prevent this header from being included more than once during a single
/// compilation without this, repeatedly including the same header would cause
/// definitions to appear multiple times

#pragma once

// gives us fixed-width integer types such as std::int_64t -> signed 64 bit
// integer, std::uint64_t -> unsighed 64-bit integer
#include <cstdint>

// a namespace is a way of organising names in C++ everything here belongs to
// mercury::market. this prevents generic names such as Price or side from being
// placed in the global namespace
namespace mercury::market {

// Prices are represented as integer tick counts
// Example:
// If an instrument has a tick size of $0.01, then a market price of $100.01 is
// represented as 10'001 ticks using integer ticks avoids floating-point
// rounding errors in price comparison and order book operations

using Price = std::int64_t;

// quantities are also represented using integer units
// we will later define the quantity scale for each instrument
// for example, BTC could use units corresponding to satoshis or another
// exchange defined minimum quantity increment
// keeping quantity integral gives us exact arithmetic in the execution and
// matching paths

using Quantity = std::int64_t;

// exchange and internal event timestamps will eventually use nanosecond
// resolution timestamp is unsighed because time measured from an epoch cannot
// be negative in mercury's normal operating model

using Timestamp = std::uint64_t;

// the side of the limit order book;
// bid = interest to buy
// ask = interest to sell
// enum class gives us strong typing and scoped values:
// Side::bid
// Side::Ask
// rather than relying on strigs, such as "BUY" and "SELL".

enum class Side { Bid, Ask };

} // namespace mercury::market
