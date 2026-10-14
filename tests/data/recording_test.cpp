#include <gtest/gtest.h>
#include <mercury/data/recording.hpp>
#include <sstream>
using namespace mercury;
TEST(Recording, RoundTripsRawMessagesClockAndConnectionBoundaries) {
  std::stringstream stream;
  data::RecordingWriter writer{stream, {"ETH-USD", 2, 6}};
  writer.write({100, true, "{\"a\":1}"}); writer.write({101, false, "{\"b\":2}"});
  writer.write({102, true, "{\"c\":3}"});
  data::RecordingReader reader{stream};
  EXPECT_EQ(reader.instrument().product_id, "ETH-USD");
  EXPECT_EQ(reader.instrument().quantity_decimals, 6U);
  auto first = reader.next(); ASSERT_TRUE(first);
  EXPECT_EQ(first->raw, "{\"a\":1}"); EXPECT_TRUE(first->new_connection);
  EXPECT_FALSE(reader.next()->new_connection); EXPECT_TRUE(reader.next()->new_connection);
  EXPECT_FALSE(reader.next());
}
TEST(Recording, RejectsBackwardsClockMalformedLinesAndBrokenOutput) {
  std::stringstream stream; data::RecordingWriter writer{stream, {}};
  EXPECT_THROW(writer.write({1, false, "x"}), std::invalid_argument);
  writer.write({2, true, "x"});
  EXPECT_THROW(writer.write({1, false, "x"}), std::invalid_argument);
  stream << "{broken\n";
  data::RecordingReader reader{stream}; ASSERT_TRUE(reader.next());
  EXPECT_THROW((void)reader.next(), std::invalid_argument);
  std::ostringstream broken; broken.setstate(std::ios::badbit);
  EXPECT_THROW((data::RecordingWriter{broken, {}}), std::runtime_error);
}
