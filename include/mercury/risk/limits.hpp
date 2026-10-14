#pragma once
#include <mercury/risk/account.hpp>
#include <algorithm>
namespace mercury::risk {
struct Limits {
  market::Quantity max_order_quantity{100'000};
  market::Quantity max_position{1'000'000};
  std::size_t max_open_orders{2};
  long double max_order_notional{1000};
  long double max_drawdown{100};
};
class RiskManager {
public:
  explicit RiskManager(Limits limits = {}) : limits_{limits} {
    if (limits.max_order_quantity <= 0 || limits.max_position < limits.max_order_quantity ||
        limits.max_position > std::numeric_limits<market::Quantity>::max() / 2 || limits.max_open_orders == 0 ||
        !std::isfinite(limits.max_order_notional) || limits.max_order_notional <= 0 ||
        !std::isfinite(limits.max_drawdown) || limits.max_drawdown <= 0)
      throw std::invalid_argument{"Invalid risk limits"};
  }
  [[nodiscard]] bool allow(market::Side side, market::Price price, market::Quantity quantity,
                          const Account& account, const execution::OrderManager& orders, bool feed_ready) const {
    if (halted_ || !feed_ready || price.ticks() <= 0 || quantity <= 0 || quantity > limits_.max_order_quantity ||
        account.notional(price, quantity) > limits_.max_order_notional ||
        account.inventory() < -limits_.max_position || account.inventory() > limits_.max_position) return false;
    std::size_t open = 0;
    auto buys = account.inventory(), sells = account.inventory();
    const auto reserve = [&](market::Side order_side, market::Quantity remaining) {
      if (order_side == market::Side::Bid) {
        if (remaining > limits_.max_position - buys) return false;
        buys += remaining;
      } else {
        if (remaining > sells + limits_.max_position) return false;
        sells -= remaining;
      }
      return true;
    };
    for (const auto& [id, order] : orders.orders()) {
      (void)id;
      if (order.terminal()) continue;
      ++open;
      if (!reserve(order.side, order.remaining())) return false;
    }
    return open < limits_.max_open_orders && reserve(side, quantity);
  }
  void observe(long double equity) {
    if (!std::isfinite(equity)) throw std::invalid_argument{"Nonfinite equity"};
    peak_equity_ = std::max(peak_equity_, equity);
    max_drawdown_ = std::max(max_drawdown_, peak_equity_ - equity);
    if (max_drawdown_ >= limits_.max_drawdown) halted_ = true;
  }
  void kill() noexcept { halted_ = true; }
  [[nodiscard]] bool halted() const noexcept { return halted_; }
  [[nodiscard]] long double max_drawdown() const noexcept { return max_drawdown_; }
private:
  Limits limits_;
  bool halted_{false};
  long double peak_equity_{0}, max_drawdown_{0};
};
} // namespace mercury::risk
