#include <gtest/gtest.h>
#include <mercury/feed/feed_session.hpp>
using namespace mercury;
namespace {
feed::FeedMessage snapshot(std::uint64_t seq = 0) {
  return {"l2_data", seq, 1, {{true, {{1, market::Side::Bid, market::Price{100}, 10},
                                     {1, market::Side::Ask, market::Price{102}, 10}}}}, {}};
}
}
TEST(FeedSession, RequiresSnapshotAndClearsOldLevels) {
  feed::FeedSession session;
  auto update = snapshot(); update.books[0].snapshot = false;
  EXPECT_EQ(session.consume(update, 1), feed::SessionResult::Applied);
  EXPECT_FALSE(session.ready());
  EXPECT_EQ(session.consume(snapshot(1), 2), feed::SessionResult::Applied);
  EXPECT_TRUE(session.ready());
  auto fresh = snapshot(2); fresh.books[0].updates[0].price = market::Price{99};
  session.consume(fresh, 3);
  EXPECT_FALSE(session.book().quantity_at(market::Side::Bid, market::Price{100}));
}
TEST(FeedSession, ChecksGlobalSequenceAndInvalidatesOnGap) {
  feed::FeedSession session;
  session.consume(snapshot(), 1);
  EXPECT_EQ(session.consume({"heartbeats", 1, 1, {}, {}}, 2), feed::SessionResult::Applied);
  EXPECT_EQ(session.consume(snapshot(1), 3), feed::SessionResult::Ignored);
  EXPECT_EQ(session.consume(snapshot(4), 4), feed::SessionResult::Gap);
  EXPECT_FALSE(session.ready());
  EXPECT_TRUE(session.book().empty(market::Side::Bid));
  session.consume(snapshot(5), 5); EXPECT_TRUE(session.ready());
}
TEST(FeedSession, StalenessResetAndCrossedBookRequireResync) {
  feed::FeedSession session{10}; session.consume(snapshot(), 100);
  EXPECT_FALSE(session.expire(110)); EXPECT_TRUE(session.expire(111));
  EXPECT_FALSE(session.ready());
  auto crossed = snapshot(); crossed.books[0].updates[0].price = market::Price{102};
  EXPECT_THROW(session.consume(crossed, 200), std::invalid_argument);
  EXPECT_FALSE(session.ready());
}
TEST(FeedSession, TradeSnapshotsAndDuplicatesCannotProduceFills) {
  feed::FeedSession session; session.consume(snapshot(), 1);
  feed::FeedMessage trades{"market_trades", 1, 1, {}, {{true, {{"a", {1, market::AggressorSide::Sell, market::Price{100}, 3}}}}}};
  session.consume(trades, 2); EXPECT_TRUE(session.trades().empty());
  trades.sequence = 2; trades.trades[0].snapshot = false;
  session.consume(trades, 3); EXPECT_TRUE(session.trades().empty());
  trades.sequence = 3; trades.trades[0].trades[0].trade_id = "b";
  session.consume(trades, 4); EXPECT_EQ(session.trades().size(), 1U);
  session.reset(); EXPECT_FALSE(session.ready());
}
