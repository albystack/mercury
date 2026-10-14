#include <gtest/gtest.h>
#include <mercury/simulation/exchange.hpp>
using namespace mercury;
namespace {
book::OrderBook make_book() {
  book::OrderBook b; b.update(market::Side::Bid, market::Price{100}, 10);
  b.update(market::Side::Ask, market::Price{102}, 10); return b;
}
market::Trade sell(market::Quantity q) { return {0, market::AggressorSide::Sell, market::Price{100}, q}; }
}
TEST(SimulatedExchange, LatencyQueueConsumptionAndPartialFills) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {10, 5}};
  const auto id = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.on_trade(sell(20), 9); EXPECT_TRUE(exchange.fills().empty());
  exchange.advance(10); EXPECT_EQ(exchange.queue_ahead(id), 10);
  exchange.on_trade(sell(12), 11);
  EXPECT_EQ(exchange.orders().get(id).filled, 2); EXPECT_EQ(exchange.queue_ahead(id), 0);
  exchange.on_trade(sell(3), 12); EXPECT_EQ(exchange.orders().get(id).remaining(), 0);
}
TEST(SimulatedExchange, SharedQueueConservesTradeVolumeAcrossOrders) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {0, 0}};
  const auto a = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  const auto b = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.advance(0); EXPECT_EQ(exchange.queue_ahead(b), 15);
  exchange.on_trade(sell(17), 1);
  EXPECT_EQ(exchange.orders().get(a).filled, 5); EXPECT_EQ(exchange.orders().get(b).filled, 2);
  market::Quantity total = 0; for (const auto& fill : exchange.fills()) total += fill.quantity;
  EXPECT_EQ(total, 7);
}
TEST(SimulatedExchange, CancellationRaceAndPostOnlyRejection) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {1, 5}};
  const auto id = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.advance(1); exchange.cancel(id, 2); exchange.on_trade(sell(12), 3);
  EXPECT_EQ(exchange.orders().get(id).filled, 2);
  exchange.advance(7); EXPECT_EQ(exchange.orders().get(id).state, execution::OrderState::Cancelled);
  const auto crossed = exchange.submit(market::Side::Bid, market::Price{102}, 5, 8);
  exchange.advance(9); EXPECT_EQ(exchange.orders().get(crossed).state, execution::OrderState::Rejected);
}
TEST(SimulatedExchange, CancelledEarlierOrderDoesNotBlockLaterOne) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {0, 0}};
  const auto a = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  const auto b = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.advance(0); exchange.cancel(a, 1); exchange.advance(1);
  EXPECT_EQ(exchange.queue_ahead(b), 10);
  exchange.on_trade(sell(15), 2); EXPECT_EQ(exchange.orders().get(b).filled, 5);
}
TEST(SimulatedExchange, ExactPriceSideAndResyncBoundaries) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {10, 5}};
  const auto id = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.invalidate(1); exchange.advance(10);
  EXPECT_TRUE(exchange.orders().get(id).terminal());
  const auto live = exchange.submit(market::Side::Bid, market::Price{100}, 5, 11);
  exchange.advance(21);
  exchange.on_trade({0, market::AggressorSide::Buy, market::Price{100}, 30}, 22);
  exchange.on_trade({0, market::AggressorSide::Sell, market::Price{99}, 30}, 23);
  EXPECT_TRUE(exchange.fills().empty());
  exchange.invalidate(24); EXPECT_TRUE(exchange.orders().get(live).terminal());
}

TEST(SimulatedExchange, DelayedTradeCannotFillOrAdvanceQueueOfNewerOrder) {
  auto book = make_book(); simulation::SimulatedExchange exchange{book, {10, 5}};
  const auto id = exchange.submit(market::Side::Bid, market::Price{100}, 5, 0);
  exchange.advance(10);
  exchange.on_trade(sell(100), 20, 9);
  EXPECT_TRUE(exchange.fills().empty()); EXPECT_EQ(exchange.queue_ahead(id), 10);
  exchange.on_trade(sell(15), 21, 11); EXPECT_EQ(exchange.orders().get(id).filled, 5);
}
