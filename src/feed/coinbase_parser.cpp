#include <mercury/feed/coinbase_parser.hpp>
#include <mercury/feed/conversion.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
namespace mercury::feed {
namespace {
std::string string_field(const nlohmann::json& object, const char* key) {
  return object.at(key).get<std::string>();
}
const nlohmann::json& array_field(const nlohmann::json& object, const char* key) {
  const auto& value = object.at(key);
  if (!value.is_array()) throw std::invalid_argument{"Expected JSON array"};
  return value;
}
}
CoinbaseParser::CoinbaseParser(Instrument instrument) : instrument_{std::move(instrument)} {
  if (instrument_.product_id.empty() || instrument_.price_decimals > 9 || instrument_.quantity_decimals > 9)
    throw std::invalid_argument{"Invalid instrument"};
}
FeedMessage CoinbaseParser::parse(std::string_view text) const {
  try {
    const auto root = nlohmann::json::parse(text);
    if (root.value("type", std::string{}) == "error") throw std::invalid_argument{"Coinbase feed error"};
    const auto& sequence = root.at("sequence_num");
    if (!sequence.is_number_unsigned()) throw std::invalid_argument{"Invalid sequence number"};
    FeedMessage result{string_field(root, "channel"), sequence.get<std::uint64_t>(),
                       utc_nanoseconds(string_field(root, "timestamp")), {}, {}};
    if (result.channel == "market_trades") {
      for (const auto& event : array_field(root, "events")) {
        const auto type = string_field(event, "type");
        if (type != "snapshot" && type != "update") throw std::invalid_argument{"Unknown trade event"};
        TradeBatch batch{type == "snapshot", {}};
        for (const auto& trade : array_field(event, "trades")) {
          if (string_field(trade, "product_id") != instrument_.product_id) continue;
          const auto maker_side = string_field(trade, "side");
          if (maker_side != "BUY" && maker_side != "SELL") throw std::invalid_argument{"Invalid trade side"};
          const auto price = decimal_units(string_field(trade, "price"), instrument_.price_decimals);
          const auto quantity = decimal_units(string_field(trade, "size"), instrument_.quantity_decimals);
          const auto id = string_field(trade, "trade_id");
          if (price <= 0 || quantity <= 0 || id.empty()) throw std::invalid_argument{"Invalid trade"};
          batch.trades.push_back({id, {utc_nanoseconds(string_field(trade, "time")),
              maker_side == "BUY" ? market::AggressorSide::Sell : market::AggressorSide::Buy,
              market::Price{price}, quantity}});
        }
        result.trades.push_back(std::move(batch));
      }
      return result;
    }
    if (result.channel != "l2_data") return result;
    for (const auto& event : array_field(root, "events")) {
      if (string_field(event, "product_id") != instrument_.product_id) continue;
      const auto type = string_field(event, "type");
      if (type != "snapshot" && type != "update") throw std::invalid_argument{"Unknown book event"};
      BookBatch batch{type == "snapshot", {}};
      for (const auto& update : array_field(event, "updates")) {
        const auto side = string_field(update, "side");
        if (side != "bid" && side != "offer") throw std::invalid_argument{"Invalid book side"};
        const auto price = decimal_units(string_field(update, "price_level"), instrument_.price_decimals);
        if (price <= 0) throw std::invalid_argument{"Price must be positive"};
        auto timestamp = utc_nanoseconds(string_field(update, "event_time"));
        if (timestamp == 0 && batch.snapshot) timestamp = result.timestamp;
        batch.updates.push_back({timestamp, side == "bid" ? market::Side::Bid : market::Side::Ask,
            market::Price{price}, decimal_units(string_field(update, "new_quantity"), instrument_.quantity_decimals)});
      }
      result.books.push_back(std::move(batch));
    }
    return result;
  } catch (const nlohmann::json::exception& error) {
    throw std::invalid_argument{std::string{"Invalid Coinbase JSON: "} + error.what()};
  }
}
} // namespace mercury::feed
