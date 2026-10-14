#include <gtest/gtest.h>
#include <mercury/feed/coinbase_parser.hpp>
#include <mercury/feed/conversion.hpp>
#include <nlohmann/json.hpp>
#include <limits>
using namespace mercury;
namespace {
nlohmann::json book_message() {
  return nlohmann::json::parse(R"({"channel":"l2_data","sequence_num":0,"timestamp":"2026-10-14T12:00:00Z","events":[{"type":"snapshot","product_id":"BTC-USD","updates":[{"side":"bid","price_level":"100.01","new_quantity":"0.00000001","event_time":"1970-01-01T00:00:00Z"},{"side":"offer","price_level":"100.02","new_quantity":"0","event_time":"2026-10-14T12:00:00.123456789Z"}]}]})");
}
}
TEST(DecimalConversion, ExactScalesAndTrailingZeros) {
  EXPECT_EQ(feed::decimal_units("100.01", 2), 10001);
  EXPECT_EQ(feed::decimal_units("0.00000001", 8), 1);
  EXPECT_EQ(feed::decimal_units("12.3400", 2), 1234);
  EXPECT_EQ(feed::decimal_units("12", 2), 1200);
  EXPECT_EQ(feed::decimal_units("9223372036854775807", 0), std::numeric_limits<std::int64_t>::max());
}
TEST(DecimalConversion, RejectsPrecisionSyntaxAndOverflow) {
  for (auto value : {"", "-1", "+1", "1e2", ".1", "1.", "1..2", "nan", "1.001"})
    EXPECT_THROW((void)feed::decimal_units(value, 2), std::invalid_argument);
  EXPECT_THROW((void)feed::decimal_units("9223372036854775808", 0), std::out_of_range);
  EXPECT_THROW((void)feed::decimal_units("92233720368547759", 2), std::out_of_range);
}
TEST(TimestampConversion, ExactUtcAndCalendarValidation) {
  EXPECT_EQ(feed::utc_nanoseconds("1970-01-01T00:00:01.123456789Z"), 1123456789ULL);
  EXPECT_THROW((void)feed::utc_nanoseconds("2026-02-30T00:00:00Z"), std::invalid_argument);
  EXPECT_THROW((void)feed::utc_nanoseconds("2026-10-14T24:00:00Z"), std::invalid_argument);
  EXPECT_THROW((void)feed::utc_nanoseconds("2026-10-14T00:00:00+01:00"), std::invalid_argument);
}
TEST(CoinbaseParser, SnapshotHasExactPricesSidesAndZeroDeletion) {
  const auto parsed = feed::CoinbaseParser{}.parse(book_message().dump());
  ASSERT_EQ(parsed.books.size(), 1U);
  EXPECT_TRUE(parsed.books[0].snapshot);
  ASSERT_EQ(parsed.books[0].updates.size(), 2U);
  EXPECT_EQ(parsed.books[0].updates[0].price.ticks(), 10001);
  EXPECT_EQ(parsed.books[0].updates[0].quantity, 1);
  EXPECT_EQ(parsed.books[0].updates[0].timestamp, parsed.timestamp);
  EXPECT_EQ(parsed.books[0].updates[1].side, market::Side::Ask);
  EXPECT_EQ(parsed.books[0].updates[1].quantity, 0);
}
TEST(CoinbaseParser, FiltersProductsAndPreservesUpdateKind) {
  auto j = book_message();
  j["events"][0]["product_id"] = "ETH-USD";
  EXPECT_TRUE(feed::CoinbaseParser{}.parse(j.dump()).books.empty());
  j["events"][0]["product_id"] = "BTC-USD";
  j["events"][0]["type"] = "update";
  EXPECT_FALSE(feed::CoinbaseParser{}.parse(j.dump()).books[0].snapshot);
}
TEST(CoinbaseParser, RejectsMalformedMessagesBeforeReturningAnyBatch) {
  EXPECT_THROW((void)feed::CoinbaseParser{}.parse("{"), std::invalid_argument);
  auto j = book_message();
  j["events"][0]["updates"][1]["side"] = "sell";
  EXPECT_THROW((void)feed::CoinbaseParser{}.parse(j.dump()), std::invalid_argument);
  j = book_message(); j["sequence_num"] = -1;
  EXPECT_THROW((void)feed::CoinbaseParser{}.parse(j.dump()), std::invalid_argument);
  j = book_message(); j["events"][0]["updates"] = nullptr;
  EXPECT_THROW((void)feed::CoinbaseParser{}.parse(j.dump()), std::invalid_argument);
}

TEST(CoinbaseParser, InvertsMakerSideAndPreservesTradeIdentityAndSnapshot) {
  const auto text = R"({"channel":"market_trades","sequence_num":1,"timestamp":"2026-10-14T12:00:00Z","events":[{"type":"snapshot","trades":[{"trade_id":"abc","product_id":"BTC-USD","price":"100.00","size":"0.1","side":"BUY","time":"2026-10-14T12:00:00Z"},{"trade_id":"def","product_id":"BTC-USD","price":"100.01","size":"0.2","side":"SELL","time":"2026-10-14T12:00:00Z"}]}]})";
  auto j = nlohmann::json::parse(text);
  const auto parsed = feed::CoinbaseParser{}.parse(text);
  ASSERT_EQ(parsed.trades.size(), 1U);
  EXPECT_TRUE(parsed.trades[0].snapshot);
  ASSERT_EQ(parsed.trades[0].trades.size(), 2U);
  EXPECT_EQ(parsed.trades[0].trades[0].trade_id, "abc");
  EXPECT_EQ(parsed.trades[0].trades[0].trade.aggressor_side, market::AggressorSide::Sell);
  EXPECT_EQ(parsed.trades[0].trades[1].trade.aggressor_side, market::AggressorSide::Buy);
  j["events"][0]["trades"][0]["size"] = "0";
  EXPECT_THROW((void)feed::CoinbaseParser{}.parse(j.dump()), std::invalid_argument);
  j["events"][0]["trades"][0]["product_id"] = "ETH-USD";
  EXPECT_EQ(feed::CoinbaseParser{}.parse(j.dump()).trades[0].trades.size(), 1U);
}
