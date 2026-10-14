#include <gtest/gtest.h>
#include <mercury/execution/orders.hpp>
using namespace mercury;
TEST(Orders, NewAcceptancePartialAndFullFill) {
  execution::OrderManager manager;
  const auto id = manager.submit(market::Side::Bid, market::Price{100}, 10, 1);
  EXPECT_THROW(manager.fill(id, 1, 1), std::logic_error);
  manager.accept(id, 2); manager.fill(id, 4, 3);
  EXPECT_EQ(manager.get(id).state, execution::OrderState::PartiallyFilled);
  EXPECT_EQ(manager.get(id).remaining(), 6);
  manager.fill(id, 6, 4); EXPECT_TRUE(manager.get(id).terminal());
  EXPECT_FALSE(manager.request_cancel(id, 5));
}
TEST(Orders, FillsRemainPossibleWhileCancelIsInFlight) {
  execution::OrderManager manager;
  const auto id = manager.submit(market::Side::Ask, market::Price{102}, 10, 1);
  manager.accept(id, 2); EXPECT_TRUE(manager.request_cancel(id, 3));
  manager.fill(id, 3, 4); EXPECT_EQ(manager.get(id).state, execution::OrderState::PendingCancel);
  EXPECT_TRUE(manager.acknowledge_cancel(id, 5));
  EXPECT_EQ(manager.get(id).filled, 3);
  EXPECT_THROW(manager.fill(id, 1, 6), std::logic_error);
}
TEST(Orders, CancelBeforeAcceptanceAndFillBeforeCancelAck) {
  execution::OrderManager manager;
  const auto id = manager.submit(market::Side::Bid, market::Price{100}, 2, 1);
  manager.request_cancel(id, 2); manager.accept(id, 3); manager.fill(id, 2, 4);
  EXPECT_FALSE(manager.acknowledge_cancel(id, 5));
  EXPECT_EQ(manager.get(id).state, execution::OrderState::Filled);
}
TEST(Orders, InvalidTransitionsAndOverfillDoNotMutateOrder) {
  execution::OrderManager manager;
  EXPECT_THROW(manager.submit(market::Side::Bid, market::Price{0}, 1, 1), std::invalid_argument);
  const auto id = manager.submit(market::Side::Bid, market::Price{100}, 5, 1);
  EXPECT_THROW(manager.acknowledge_cancel(id, 1), std::logic_error);
  manager.accept(id, 2);
  EXPECT_THROW(manager.accept(id, 3), std::logic_error);
  EXPECT_THROW(manager.fill(id, 6, 3), std::invalid_argument);
  EXPECT_THROW(manager.fill(id, 1, 1), std::invalid_argument);
  EXPECT_EQ(manager.get(id).filled, 0);
  const auto rejected = manager.submit(market::Side::Ask, market::Price{102}, 5, 3);
  manager.reject(rejected, 4); EXPECT_TRUE(manager.get(rejected).terminal());
}
