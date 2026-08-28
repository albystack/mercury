/// prevent this header from being included more than once during a single
/// compilation without this, repeatedly including the same header would cause
/// definitions to appear multiple times

#pragma once

// gives us fixed-width integer types such as std::int_64t -> signed 64 bit
// integer, std::uint64_t -> unsighed 64-bit integer
#include <cstdint>

#include <string_view>

#include <compare>
// provides the machinery used by modern C++ comparisons

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

// price (new)
//
// we had price as std::int64_t, as far as c++ were concerned, they were the
// same this is dangerous, as mercury contains many different numerical values
// like price, quantity, timestamp a trading engine should make invalid
// combinations difficult to express, so now price has its own class
class Price {
public:
  // constructor
  // a constructor creates a price obkject
  // python:
  // class Price:
  // def __init__(self,ticks);
  // self.ticks = ticks
  // in c++ its price(std::int64_t ticks)
  //
  // notice a constructor has the same name as the class and does not have a
  // return type
  //
  // "explicit" prevents c++ from silently converting an integer into a price
  // we want Price price{10'001};
  // we dont want implicit conversions.
  // : ticks_{ticks} is a member initialiser list. the parameter ticks is used
  // to initialise the objects private member ticks_
  //
  explicit constexpr Price(std::int64_t ticks) : ticks_{ticks} {}

  // accessor
  // this function lets other code read the raw number of ticks stored inside
  // the Price
  // [[nodiscard]] tells the compiler the return value of this function is
  // important the const after the function means calling this function will not
  // modify this price object
  [[nodiscard]]
  constexpr std::int64_t ticks() const noexcept {
    return ticks_;
  }

  // comparison operators
  // equality comparison this allows Price a{100} Price b{100}, a==b is True
  // const Price& other
  // other is reference to another Price
  // "&" means we refer to the existing object rather than copying it
  // "const" means we promise not to modify the object
  // the second const means the function also promises not to modify the Price
  // object that the function is being called on
  //
  [[nodiscard]]
  constexpr bool operator==(const Price &other) const noexcept = default;

  // three way comparison operator
  // this operator <=> is the spaceship operator
  // by defining it, c++ can provide the normal ordering operations: < <= > >=
  // based on the ordering of Price's data members
  //
  // since price only stores ticks_ this effectively means lower_ticks -> lower
  // Price, higher ticks_ -> Higher Price = default tells the compiler to
  // generate the implementation
  [[nodiscard]]
  constexpr auto operator<=>(const Price &other) const noexcept = default;

private:
  // intenral representation
  // this is the actual integer stored inside every Price
  // it is PRIVATE. code outside price cannot do price.ticks_ = 500
  // instead, price controls how its intenral state can be accessed
  std::int64_t ticks_;
};

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
