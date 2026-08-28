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
