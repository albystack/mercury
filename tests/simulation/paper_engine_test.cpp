#include <gtest/gtest.h>
#include <mercury/simulation/paper_engine.hpp>
#include <fstream>
#include <sstream>
using namespace mercury;
namespace {
struct Result { std::size_t fills, orders; market::Quantity inventory; long double equity, fees; bool halted; };
Result run_sample(simulation::PaperConfig config = {}) {
  std::ifstream input{std::string{MERCURY_FIXTURE_DIR} + "/sample.jsonl"};
  data::RecordingReader reader{input}; feed::FeedSession session;
  simulation::PaperEngine engine{session, reader.instrument(), config};
  data::replay(reader, session,
    [&](const auto& record, const auto&, const auto& state) { engine.observe(record, state); },
    [&](const auto& record, const auto& message, const auto& state) { engine.before(record, message, state); });
  engine.finish();
  for (const auto& [id, order] : engine.exchange().orders().orders()) { (void)id; EXPECT_TRUE(order.terminal()); }
  return {engine.account().fill_count(), engine.submitted(), engine.account().inventory(),
          engine.equity(), engine.account().fees(), engine.risk_manager().halted()};
}
}
TEST(PaperEngine, GoldenReplayIsDeterministicAndConservesCashAndInventory) {
  const auto first = run_sample(), second = run_sample();
  EXPECT_EQ(first.fills, 3U); EXPECT_EQ(first.orders, 4U); EXPECT_EQ(first.inventory, 0);
  EXPECT_NEAR(static_cast<double>(first.equity), 0.00002, 1e-12);
  EXPECT_EQ(first.fills, second.fills); EXPECT_EQ(first.equity, second.equity);
}
TEST(PaperEngine, FeesReduceEquityAndSlowOrdersMissTrades) {
  simulation::PaperConfig fees; fees.fee_bps = 10;
  const auto baseline = run_sample(), with_fees = run_sample(fees);
  EXPECT_LT(with_fees.equity, baseline.equity);
  EXPECT_NEAR(static_cast<double>(baseline.equity - with_fees.equity), static_cast<double>(with_fees.fees), 1e-12);
  simulation::PaperConfig slow; slow.latency = {1'000'000'000, 1'000'000'000};
  EXPECT_EQ(run_sample(slow).fills, 0U);
}
TEST(PaperEngine, LossHaltCancelsExposureAndDoesNotResume) {
  simulation::PaperConfig config; config.fee_bps = 10'000; config.limits.max_drawdown = 0.00001L;
  const auto result = run_sample(config); EXPECT_TRUE(result.halted); EXPECT_EQ(result.fills, 1U);
}
TEST(PaperEngine, ActivationUsesBookBeforeNewArrivalNotFutureQuantity) {
  std::ifstream input{std::string{MERCURY_FIXTURE_DIR} + "/sample.jsonl"};
  data::RecordingReader source{input}; const auto snapshot = source.next(); ASSERT_TRUE(snapshot);
  std::stringstream stream; data::RecordingWriter writer{stream, source.instrument()}; writer.write(*snapshot);
  writer.write({1'100'000'000, false, R"({"channel":"l2_data","sequence_num":1,"timestamp":"2026-10-14T12:00:00.100Z","events":[{"type":"update","product_id":"BTC-USD","updates":[{"side":"bid","price_level":"100.00","new_quantity":"0.01","event_time":"2026-10-14T12:00:00Z"}]}]})"});
  writer.write({1'200'000'000, false, R"({"channel":"market_trades","sequence_num":2,"timestamp":"2026-10-14T12:00:00.200Z","events":[{"type":"update","trades":[{"trade_id":"x","product_id":"BTC-USD","side":"BUY","price":"100.00","size":"0.0015","time":"2026-10-14T12:00:00.200Z"}]}]})"});
  data::RecordingReader reader{stream}; feed::FeedSession session;
  simulation::PaperConfig config; config.latency = {50'000'000, 50'000'000};
  simulation::PaperEngine engine{session, reader.instrument(), config};
  data::replay(reader, session,
    [&](const auto& record, const auto&, const auto& state) { engine.observe(record, state); },
    [&](const auto& record, const auto& message, const auto& state) { engine.before(record, message, state); });
  EXPECT_EQ(engine.account().inventory(), 50'000);
}
TEST(PaperEngine, GapRemovesSimulatedExposureAndCannotFillFromStaleBook) {
  feed::FeedSession session;
  feed::FeedMessage first{"l2_data", 0, 1, {{true, {{1, market::Side::Bid, market::Price{10000}, 100000},
                                                {1, market::Side::Ask, market::Price{10002}, 100000}}}}, {}};
  simulation::PaperEngine engine{session, {}};
  data::Record record{1, true, "x"}; engine.before(record, first, session);
  session.consume(first, 1); engine.observe(record, session);
  feed::FeedMessage gap{"heartbeats", 3, 1, {}, {}};
  record = {2'000'000, false, "x"}; engine.before(record, gap, session);
  EXPECT_EQ(session.consume(gap, record.received_ns), feed::SessionResult::Gap);
  engine.observe(record, session);
  for (const auto& [id, order] : engine.exchange().orders().orders()) { (void)id; EXPECT_TRUE(order.terminal()); }
  EXPECT_EQ(engine.account().fill_count(), 0U);
}

TEST(PaperEngine, RepricingWaitsForCancellationAndKeepsExposureBounded) {
  feed::FeedSession session; simulation::PaperConfig config;
  config.latency = {10, 10};
  simulation::PaperEngine engine{session, {}, config};
  feed::FeedMessage message{"l2_data", 0, 1, {{true, {{1, market::Side::Bid, market::Price{10000}, 100000},
                                                  {1, market::Side::Ask, market::Price{10002}, 100000}}}}, {}};
  const auto process = [&](market::Timestamp now) {
    data::Record record{now, now == 1, "x"}; engine.before(record, message, session);
    session.consume(message, now); engine.observe(record, session);
  };
  process(1); message.sequence = 1; message.books.clear(); message.channel = "heartbeats"; process(12);
  message.sequence = 2; message.channel = "l2_data";
  message.books = {{false, {{1, market::Side::Bid, market::Price{10000}, 0},
                           {1, market::Side::Ask, market::Price{10002}, 0},
                           {1, market::Side::Bid, market::Price{10001}, 100000},
                           {1, market::Side::Ask, market::Price{10003}, 100000}}}};
  process(13); EXPECT_EQ(engine.submitted(), 2U); EXPECT_EQ(engine.cancellations(), 2U);
  message.sequence = 3; message.books.clear(); message.channel = "heartbeats"; process(14);
  EXPECT_EQ(engine.submitted(), 2U);
  message.sequence = 4; process(23); EXPECT_EQ(engine.submitted(), 4U);
  std::size_t open = 0;
  for (const auto& [id, order] : engine.exchange().orders().orders()) { (void)id; if (!order.terminal()) ++open; }
  EXPECT_EQ(open, 2U);
  engine.kill(24); EXPECT_TRUE(engine.risk_manager().halted());
  message.sequence = 5; process(40); EXPECT_EQ(engine.submitted(), 4U);
}
