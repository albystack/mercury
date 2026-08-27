/// prevent this header from being included more than once during a single
/// compilation without this, repeatedly including the same header would cause
/// definitions to appear multiple times

#pragma once

// gives us fixed-width integer types such as std::int_64t -> signed 64 bit
// integer, std::uint64_t -> unsighed 64-bit integer
#include <cstdint>

#include <string_view>

// why string view? in python, a string is just "Bid" in c++ there are several
// string related types. A full std::string owns memory and can modify its
// contents. but "Bid" is just a fixed string literal. we dont need to allocate
// or copy anything std::string_view is essentially a lightweight read online
// view over some characters, no heap allocation

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

// side -> text conversion
//
// convert a Side value into human-readable text. Example: to_string(Side::Bid)
// returns: "Bid"
//
// The function returns std::string_view rather than std::string because the
// returned values are fixed string literals the function is constexpr which
// means C++ is allowed to evaluate it during compilationn when its input is
// known at compile time.

constexpr std::string_view to_string(Side side) {

  // switch chooses one branch based on the value of side
  //
  // ptyhon eq:
  // if side == Side.bid
  // return bid
  // elif side = side.ask
  // return ask
  // in C++, an enumerator is often naturally handled with switch
  //
  switch (side) {
  case Side::Bid:
    return "Bid";

  case Side::Ask:
    return "Ask";
  }
  // in normal mercury code we should never reach this line because side
  // currently has only bid and ask we still return something so that the
  // function has a valid return value for every possible underlying enum
  // representation
  return "Unknown";
}

} // namespace mercury::market
