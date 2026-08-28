// Mercury market-type unit tests

// This file verifies the behaviour and invariants of Mercury's fundamental
// market domain types Unlike apps/mercury.app, this file is not an application
//
// Its purpose is to test things. Does Price preserve its tick value, are two
// equal Prices recognised as equal, are prices ordered cocrrectly, does side
// convert to correct text, and so on

// GoogleTest's main header
// This gives us
//  TEST
//  EXPECT_EQ
//  EXPECT_LT
//  EXPECT_GT
//  and many more
#include <cstdint>
#include <gtest/gtest.h>

// We use this to inspect properties of C++ types at compilation time
#include <type_traits>

// Mercury's domain types
#include <mercury/market/types.hpp>

// Compile time safety checks
// bring the Price type into this file's scope
using mercury::market::Price;

// Our Price constructor is explicit.
// THerefore an int64_t must NOT be implicitly convertible into Price
// if someone removes "explicit" from the Price constructor in future,
// compilation will fail static_assert is evaluated by compiler rather than
// while the program runs
static_assert(!std::is_convertible_v<std::int64_t, Price>,
              "Price must not allow implicit conversion from int64_t");

// Explicit consturction should still be valid. This verifies Price{100} is
// legal
static_assert(std::is_constructible_v<Price, std::int64_t>,
              "Price must be explicitly constructivle from int64_t");

// Pricing tests
// TEST creates an individual GoogleTest test
// Syntax is: TEST(TestSuiteName, TestName)
// Here: PriceTest groups related PriceTests
// StoresTickValue describes exactly what this particular test verifies
TEST(PriceTest, StoresTickValue) {
  // Arrange / construct the object being tested.
  constexpr Price price{10'001};

  // EXPECT_EQ means:
  //
  //     expect these two expressions to be equal
  //
  // If they are not equal, GoogleTest reports a failed test rather than
  // silently continuing.
  EXPECT_EQ(price.ticks(), 10'001);
}

// Verify Price equality.
TEST(PriceTest, EqualTickValuesCompareEqual) {
  constexpr Price first{10'000};
  constexpr Price second{10'000};

  EXPECT_EQ(first, second);
}

// Verify that different tick values are not equal.
TEST(PriceTest, DifferentTickValuesCompareUnequal) {
  constexpr Price first{10'000};
  constexpr Price second{10'001};

  EXPECT_NE(first, second);
}

// Verify the ordering introduced through operator<=>.
//
// In market terms, imagine:
//
//     bid = $100.00
//     ask = $100.01
//
// where the tick size is $0.01.
TEST(PriceTest, OrdersPricesByTickValue) {
  constexpr Price bid{10'000};
  constexpr Price ask{10'001};

  EXPECT_LT(bid, ask);
  EXPECT_GT(ask, bid);
}

// -----------------------------------------------------------------------------
// Side tests
// -----------------------------------------------------------------------------

TEST(SideTest, ConvertsBidToText) {
  EXPECT_EQ(mercury::market::to_string(mercury::market::Side::Bid), "Bid");
}

TEST(SideTest, ConvertsAskToText) {
  EXPECT_EQ(mercury::market::to_string(mercury::market::Side::Ask), "Ask");
}
