#include <gtest/gtest.h>

#include <mercury/book/order_book.hpp>
#include <mercury/market/events.hpp>
#include <mercury/market/market_data_processor.hpp>

using mercury::book::OrderBook;
using mercury::market::AggressorSide;
using mercury::market::BookUpdate;
using mercury::market::MarketDataProcessor;
using mercury::market::MarketEvent;
using mercury::market::Price;
using mercury::market::Side;
using mercury::market::Timestamp;
using mercury::market::Trade;

TEST(MarketDataProcessorTest, AppliesBookUpdateToOrderBook) {
  OrderBook book;
  MarketDataProcessor processor{book};

  const MarketEvent event{
      BookUpdate{Timestamp{1'000'000}, Side::Bid, Price{10'000}, 500}};

  processor.process(event);

  const auto quantity = book.quantity_at(Side::Bid, Price{10'000});

  ASSERT_TRUE(quantity.has_value());
  EXPECT_EQ(quantity.value(), 500);
}
TEST(MarketDataProcessorTest, DoesNotApplyTradeDirectlyToOrderBook) {
  OrderBook book;
  MarketDataProcessor processor{book};

  book.update(Side::Bid, Price{10'000}, 500);

  const MarketEvent event{
      Trade{Timestamp{2'000'000}, AggressorSide::Sell, Price{10'000}, 100}};

  processor.process(event);

  const auto quantity = book.quantity_at(Side::Bid, Price{10'000});

  ASSERT_TRUE(quantity.has_value());
  EXPECT_EQ(quantity.value(), 500);
}

TEST(MarketDataProcessorTest, PreservesBookUpdateSemantics) {
  OrderBook book;
  MarketDataProcessor processor{book};

  processor.process(MarketEvent{
      BookUpdate{Timestamp{1'000'000}, Side::Bid, Price{10'000}, 500}});

  processor.process(MarketEvent{
      BookUpdate{Timestamp{2'000'000}, Side::Bid, Price{10'000}, 800}});

  const auto replaced_quantity = book.quantity_at(Side::Bid, Price{10'000});

  ASSERT_TRUE(replaced_quantity.has_value());
  EXPECT_EQ(replaced_quantity.value(), 800);

  processor.process(MarketEvent{
      BookUpdate{Timestamp{3'000'000}, Side::Bid, Price{10'000}, 0}});

  EXPECT_FALSE(book.quantity_at(Side::Bid, Price{10'000}).has_value());
}
