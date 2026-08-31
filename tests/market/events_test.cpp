#include <gtest/gtest.h>

#include <mercury/market/events.hpp>

using mercury::market::AggressorSide;
using mercury::market::BookUpdate;
using mercury::market::Price;
using mercury::market::Side;
using mercury::market::Timestamp;
using mercury::market::Trade;

TEST(BookUpdateTest, StoresEventFields) {
  const BookUpdate update{Timestamp{1'000'000}, Side::Bid, Price{10'000}, 500};

  EXPECT_EQ(update.timestamp, Timestamp{1'000'000});
  EXPECT_EQ(update.side, Side::Bid);
  EXPECT_EQ(update.price, Price{10'000});
  EXPECT_EQ(update.quantity, 500);
}

TEST(TradeTest, StoresTradeFields) {
  const Trade trade{Timestamp{2'000'000}, AggressorSide::Buy, Price{10'001},
                    100};

  EXPECT_EQ(trade.timestamp, Timestamp{2'000'000});
  EXPECT_EQ(trade.aggressor_side, AggressorSide::Buy);
  EXPECT_EQ(trade.price, Price{10'001});
  EXPECT_EQ(trade.quantity, 100);
}

TEST(TradeTest, StoresSellAggressor) {
  const Trade trade{Timestamp{3'000'000}, AggressorSide::Sell, Price{10'000},
                    250};

  EXPECT_EQ(trade.aggressor_side, AggressorSide::Sell);
}
