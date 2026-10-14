#include <gtest/gtest.h>
#include <mercury/risk/limits.hpp>
using namespace mercury;
TEST(Account, CashInventoryFeesAndMarkedEquity) {
  risk::Account account{{"TEST", 0, 0}, 100};
  account.apply({1, market::Side::Bid, market::Price{100}, 2, 1});
  EXPECT_EQ(account.inventory(), 2); EXPECT_DOUBLE_EQ(static_cast<double>(account.cash()), -202);
  EXPECT_DOUBLE_EQ(static_cast<double>(account.equity(market::Price{101})), 0);
  account.apply({2, market::Side::Ask, market::Price{102}, 2, 2});
  EXPECT_EQ(account.inventory(), 0);
  EXPECT_NEAR(static_cast<double>(account.cash()), -0.04, 1e-10);
  EXPECT_EQ(account.fill_count(), 2U);
}
TEST(Risk, ReservesPendingAndCancellingOrdersWithoutOffsettingSides) {
  risk::Account account{{"TEST", 0, 0}}; execution::OrderManager orders;
  account.apply({1, market::Side::Bid, market::Price{100}, 2, 1});
  risk::RiskManager risk{{7, 10, 4, 1000, 100}};
  const auto id = orders.submit(market::Side::Bid, market::Price{100}, 7, 2);
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{100}, 2, account, orders, true));
  EXPECT_TRUE(risk.allow(market::Side::Ask, market::Price{102}, 5, account, orders, true));
  orders.accept(id, 3); orders.request_cancel(id, 4);
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{100}, 2, account, orders, true));
  orders.acknowledge_cancel(id, 5);
  EXPECT_TRUE(risk.allow(market::Side::Bid, market::Price{100}, 2, account, orders, true));
}
TEST(Risk, EnforcesNotionalOrderCountFeedAndLatchedLossStop) {
  risk::Account account{{"TEST", 0, 0}}; execution::OrderManager orders;
  risk::RiskManager risk{{5, 10, 1, 500, 10}};
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{100}, 6, account, orders, true));
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{101}, 5, account, orders, true));
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{100}, 5, account, orders, false));
  orders.submit(market::Side::Ask, market::Price{102}, 1, 1);
  EXPECT_FALSE(risk.allow(market::Side::Bid, market::Price{100}, 1, account, orders, true));
  risk.observe(5); risk.observe(-5); EXPECT_TRUE(risk.halted());
  risk.observe(10); EXPECT_TRUE(risk.halted());
}
TEST(Account, RejectsOverflowAndInvalidFillsWithoutMutatingBalance) {
  risk::Account account{{"TEST", 0, 0}};
  account.apply({1, market::Side::Bid, market::Price{1}, std::numeric_limits<market::Quantity>::max(), 1});
  EXPECT_THROW(account.apply({1, market::Side::Bid, market::Price{1}, 1, 2}), std::out_of_range);
  EXPECT_EQ(account.inventory(), std::numeric_limits<market::Quantity>::max());
  EXPECT_THROW(account.apply({1, market::Side::Ask, market::Price{1}, 0, 2}), std::invalid_argument);
}
