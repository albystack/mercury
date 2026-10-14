#include <gtest/gtest.h>
#include <mercury/data/replay.hpp>
#include <fstream>
#include <sstream>
using namespace mercury;
TEST(Replay, FixedRecordingProducesIdenticalResults) {
  for (int run = 0; run < 2; ++run) {
    std::ifstream input{std::string{MERCURY_FIXTURE_DIR} + "/sample.jsonl"};
    data::RecordingReader reader{input}; feed::FeedSession session;
    const auto stats = data::replay(reader, session);
    EXPECT_EQ(stats.records, 8U); EXPECT_EQ(stats.gaps, 0U);
    EXPECT_EQ(stats.trades, 3U); EXPECT_TRUE(session.ready());
    EXPECT_EQ(session.book().best_bid()->price.ticks(), 10000);
    EXPECT_EQ(session.book().best_ask()->price.ticks(), 10002);
  }
}
TEST(Replay, ConnectionBoundariesResetBookAndSequence) {
  std::ifstream input{std::string{MERCURY_FIXTURE_DIR} + "/sample.jsonl"};
  data::RecordingReader source{input}; const auto first = source.next(); ASSERT_TRUE(first);
  std::stringstream stream; data::RecordingWriter writer{stream, source.instrument()};
  writer.write(*first); writer.write({first->received_ns + 1, true, first->raw});
  data::RecordingReader reader{stream}; feed::FeedSession session;
  const auto stats = data::replay(reader, session);
  EXPECT_EQ(stats.connections, 2U); EXPECT_TRUE(session.ready());
}
