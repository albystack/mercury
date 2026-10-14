#include <gtest/gtest.h>
#include <mercury/strategy/quotes.hpp>
using namespace mercury;
namespace {
book::OrderBook quotes_book() {
  book::OrderBook b; b.update(market::Side::Bid, market::Price{100}, 10);
  b.update(market::Side::Ask, market::Price{102}, 10); return b;
}
}
TEST(InventoryQuoter, NeutralQuotesAndInventorySkew) {
  const auto book = quotes_book(); strategy::InventoryQuoter quoter{{5, 1, 10, 20}};
  const auto neutral = quoter.quotes(book, 0);
  ASSERT_TRUE(neutral.bid); ASSERT_TRUE(neutral.ask);
  EXPECT_EQ(neutral.bid->price.ticks(), 100); EXPECT_EQ(neutral.ask->price.ticks(), 102);
  const auto long_inventory = quoter.quotes(book, 5);
  EXPECT_EQ(long_inventory.bid->price.ticks(), 99); EXPECT_EQ(long_inventory.ask->price.ticks(), 101);
  const auto short_inventory = quoter.quotes(book, -5);
  EXPECT_EQ(short_inventory.bid->price.ticks(), 101); EXPECT_EQ(short_inventory.ask->price.ticks(), 103);
}
TEST(InventoryQuoter, LimitsOnlyRemoveRiskIncreasingSide) {
  const auto book = quotes_book(); strategy::InventoryQuoter quoter{{5, 1, 10, 20}};
  const auto long_inventory = quoter.quotes(book, 20);
  EXPECT_FALSE(long_inventory.bid); EXPECT_TRUE(long_inventory.ask);
  const auto short_inventory = quoter.quotes(book, -20);
  EXPECT_TRUE(short_inventory.bid); EXPECT_FALSE(short_inventory.ask);
}
TEST(InventoryQuoter, EmptyCrossedAndExtremeBooksProduceNoQuotes) {
  strategy::InventoryQuoter quoter; book::OrderBook book;
  EXPECT_FALSE(quoter.quotes(book, 0).bid);
  book.update(market::Side::Bid, market::Price{100}, 1);
  book.update(market::Side::Ask, market::Price{100}, 1);
  EXPECT_FALSE(quoter.quotes(book, 0).ask);
  book = {}; book.update(market::Side::Bid, market::Price{std::numeric_limits<std::int64_t>::max()-1}, 1);
  book.update(market::Side::Ask, market::Price{std::numeric_limits<std::int64_t>::max()}, 1);
  EXPECT_FALSE(quoter.quotes(book, 0).bid);
}
TEST(InventoryQuoter, QuotesRemainPostOnlyUnderExtremeInventory) {
  const auto book = quotes_book(); strategy::InventoryQuoter quoter{{5, 1, 10, 20}};
  const auto q = quoter.quotes(book, std::numeric_limits<market::Quantity>::min());
  ASSERT_TRUE(q.bid); EXPECT_LT(q.bid->price, book.best_ask()->price);
  EXPECT_FALSE(q.ask);
}
