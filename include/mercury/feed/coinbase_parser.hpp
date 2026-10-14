#pragma once
#include <mercury/market/events.hpp>
#include <string>
#include <string_view>
#include <vector>
namespace mercury::feed {
struct Instrument {
  std::string product_id{"BTC-USD"};
  unsigned price_decimals{2};
  unsigned quantity_decimals{8};
};
struct BookBatch {
  bool snapshot;
  std::vector<market::BookUpdate> updates;
};
struct FeedMessage {
  std::string channel;
  std::uint64_t sequence;
  market::Timestamp timestamp;
  std::vector<BookBatch> books;
};
class CoinbaseParser {
public:
  explicit CoinbaseParser(Instrument instrument = {});
  [[nodiscard]] FeedMessage parse(std::string_view text) const;
  [[nodiscard]] const Instrument& instrument() const noexcept { return instrument_; }
private:
  Instrument instrument_;
};
} // namespace mercury::feed
