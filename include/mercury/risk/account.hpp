#pragma once
#include <mercury/feed/coinbase_parser.hpp>
#include <mercury/simulation/exchange.hpp>
#include <cmath>
namespace mercury::risk {
[[nodiscard]] inline long double decimal_scale(unsigned decimals) {
  if (decimals > 9) throw std::invalid_argument{"Unsupported accounting scale"};
  long double scale = 1; for (unsigned i = 0; i < decimals; ++i) scale *= 10;
  return scale;
}
class Account {
public:
  explicit Account(const feed::Instrument& instrument = {}, long double fee_bps = 0)
      : price_scale_{decimal_scale(instrument.price_decimals)},
        quantity_scale_{decimal_scale(instrument.quantity_decimals)}, fee_bps_{fee_bps} {
    if (!std::isfinite(fee_bps) || fee_bps < 0 || fee_bps > 10'000)
      throw std::invalid_argument{"Invalid maker fee"};
  }
  [[nodiscard]] long double notional(market::Price price, market::Quantity quantity) const noexcept {
    return static_cast<long double>(price.ticks()) / price_scale_ * static_cast<long double>(quantity) / quantity_scale_;
  }
  void apply(const simulation::Fill& fill) {
    const auto quantity = fill.quantity;
    if (quantity <= 0 || fill.price.ticks() <= 0 || (last_fill_ && fill.timestamp < *last_fill_))
      throw std::invalid_argument{"Invalid accounting fill"};
    if ((fill.side == market::Side::Bid && inventory_ > std::numeric_limits<market::Quantity>::max() - quantity) ||
        (fill.side == market::Side::Ask && inventory_ < std::numeric_limits<market::Quantity>::min() + quantity))
      throw std::out_of_range{"Inventory overflows"};
    const auto value = notional(fill.price, quantity);
    const auto fee = value * fee_bps_ / 10'000;
    inventory_ += fill.side == market::Side::Bid ? quantity : -quantity;
    cash_ += fill.side == market::Side::Bid ? -value : value;
    cash_ -= fee; fees_ += fee; turnover_ += value;
    ++fill_count_; last_fill_ = fill.timestamp;
  }
  [[nodiscard]] market::Quantity inventory() const noexcept { return inventory_; }
  [[nodiscard]] long double cash() const noexcept { return cash_; }
  [[nodiscard]] long double fees() const noexcept { return fees_; }
  [[nodiscard]] long double turnover() const noexcept { return turnover_; }
  [[nodiscard]] std::size_t fill_count() const noexcept { return fill_count_; }
  [[nodiscard]] long double equity(market::Price mark) const noexcept { return cash_ + notional(mark, inventory_); }
private:
  long double price_scale_;
  long double quantity_scale_;
  long double fee_bps_;
  market::Quantity inventory_{0};
  long double cash_{0}, fees_{0}, turnover_{0};
  std::size_t fill_count_{0};
  std::optional<market::Timestamp> last_fill_;
};
} // namespace mercury::risk
