#include "mercury/market/types.hpp"
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
