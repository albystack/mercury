#include "mercury/market/types.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <mercury/book/order_book.hpp>

using mercury::book::OrderBook;
using mercury::market::Side;

// A newly constructed order book should contain neither bids nor asks
TEST(OrderBookTest, StartsEmpty) {
  const OrderBook book;

  EXPECT_TRUE(book.empty(Side::Bid));
  EXPECT_TRUE(book.empty(Side::Ask));
}

TEST(OrderBookTest, InsertsBidLevel) {
  OrderBook book;
  const mercury::market::Price price{10'000};
  book.update(Side::Bid, price, 500);

  EXPECT_FALSE(book.empty(Side::Bid));
  EXPECT_TRUE(book.empty(Side::Ask));

  const auto quantity = book.quantity_at(Side::Bid, price);

  ASSERT_TRUE(quantity.has_value());
  EXPECT_EQ(quantity.value(), 500);
}

TEST(OrderBookTest, ReplacesExistingBidQuantity) {
  OrderBook book;
  const mercury::market::Price price{10'000};
  book.update(Side::Bid, price, 500);
  book.update(Side::Bid, price, 800);
  const auto quantity = book.quantity_at(Side::Bid, price);
  ASSERT_TRUE(quantity.has_value());
  EXPECT_EQ(quantity.value(), 800);
}

TEST(OrderBookTest, RemovesLevelWhenQuantityBecomesZero) {
  OrderBook book;
  const mercury::market::Price price{10'000};

  book.update(Side::Bid, price, 500);
  book.update(Side::Bid, price, 0);

  EXPECT_TRUE(book.empty(Side::Bid));
  EXPECT_FALSE(book.quantity_at(Side::Bid, price).has_value());
}

TEST(OrderBookTest, InsertsAskLevel) {
  OrderBook book;
  const mercury::market::Price price{10'001};
  book.update(Side::Ask, price, 300);
  EXPECT_TRUE(book.empty(Side::Bid));
  EXPECT_FALSE(book.empty(Side::Ask));

  const auto quantity = book.quantity_at(Side::Ask, price);

  ASSERT_TRUE(quantity.has_value());
  EXPECT_EQ(quantity.value(), 300);
}

TEST(OrderBookTest, RejectNegativeQuantity) {
  OrderBook book;
  const mercury::market::Price price{10'000};

  EXPECT_THROW(book.update(Side::Bid, price, -1), std::invalid_argument);

  EXPECT_TRUE(book.empty(Side::Bid));
}

TEST(OrderBookTest, HasNoBestPricesWhenEmpty) {
  const OrderBook book;
  EXPECT_FALSE(book.best_bid().has_value());
  EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBookTest, ReturnsHighestBidAsBestBid) {
  OrderBook book;

  // Deliberately insert these out of order
  book.update(Side::Bid, mercury::market::Price{9'998}, 200);
  book.update(Side::Bid, mercury::market::Price{10'000}, 500);
  book.update(Side::Bid, mercury::market::Price{9'999}, 300);

  const auto best = book.best_bid();

  ASSERT_TRUE(best.has_value());

  EXPECT_EQ(best->price, mercury::market::Price{10'000});
  EXPECT_EQ(best->quantity, 500);
}

TEST(OrderBookTest, ReturnsLowestAskAsBestAsk) {
  OrderBook book;

  // Again, deliberately insert out of order.
  book.update(Side::Ask, mercury::market::Price{10'003}, 700);
  book.update(Side::Ask, mercury::market::Price{10'001}, 300);
  book.update(Side::Ask, mercury::market::Price{10'002}, 500);

  const auto best = book.best_ask();

  ASSERT_TRUE(best.has_value());

  EXPECT_EQ(best->price, mercury::market::Price{10'001});
  EXPECT_EQ(best->quantity, 300);
}

TEST(OrderBookTest, PromotesNextBidAfterBestBidIsRemoved) {
  OrderBook book;

  const mercury::market::Price best_price{10'000};
  const mercury::market::Price next_price{9'999};

  book.update(Side::Bid, next_price, 300);
  book.update(Side::Bid, best_price, 500);

  // Quantity zero removes the current best level.
  book.update(Side::Bid, best_price, 0);

  const auto best = book.best_bid();

  ASSERT_TRUE(best.has_value());

  EXPECT_EQ(best->price, next_price);
  EXPECT_EQ(best->quantity, 300);
}

TEST(OrderBookTest, PromotesNextAskAfterBestAskIsRemoved) {
  OrderBook book;

  const mercury::market::Price best_price{10'001};
  const mercury::market::Price next_price{10'002};

  book.update(Side::Ask, next_price, 500);
  book.update(Side::Ask, best_price, 300);

  // Remove the current best ask.
  book.update(Side::Ask, best_price, 0);

  const auto best = book.best_ask();

  ASSERT_TRUE(best.has_value());

  EXPECT_EQ(best->price, next_price);
  EXPECT_EQ(best->quantity, 500);
}

TEST(OrderBookTest, KeepsBidAndAskSidesIndependent) {
  OrderBook book;

  const mercury::market::Price price{10'000};

  // The same numerical price can exist independently on both sides
  // of our local book representation.
  book.update(Side::Bid, price, 500);
  book.update(Side::Ask, price, 300);

  const auto bid_quantity = book.quantity_at(Side::Bid, price);
  const auto ask_quantity = book.quantity_at(Side::Ask, price);

  ASSERT_TRUE(bid_quantity.has_value());
  ASSERT_TRUE(ask_quantity.has_value());

  EXPECT_EQ(bid_quantity.value(), 500);
  EXPECT_EQ(ask_quantity.value(), 300);

  // Removing the bid must not remove the ask.
  book.update(Side::Bid, price, 0);

  EXPECT_FALSE(book.quantity_at(Side::Bid, price).has_value());

  const auto remaining_ask = book.quantity_at(Side::Ask, price);

  ASSERT_TRUE(remaining_ask.has_value());
  EXPECT_EQ(remaining_ask.value(), 300);
}
