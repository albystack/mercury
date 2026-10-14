#pragma once
#include <mercury/book/order_book.hpp>
#include <mercury/execution/orders.hpp>
#include <mercury/market/events.hpp>
#include <mercury/simulation/scheduler.hpp>
#include <algorithm>
#include <deque>
#include <optional>
namespace mercury::simulation {
struct Fill {
  execution::OrderId order_id;
  market::Side side;
  market::Price price;
  market::Quantity quantity;
  market::Timestamp timestamp;
};
class SimulatedExchange {
public:
  explicit SimulatedExchange(const book::OrderBook& book, Latency latency = {}) : book_{book}, latency_{latency} {}
  void advance(market::Timestamp now) { scheduler_.advance(now); }
  execution::OrderId submit(market::Side side, market::Price price, market::Quantity quantity, market::Timestamp now) {
    const auto deadline = after(now, latency_.new_order_ns);
    advance(now);
    const auto id = orders_.submit(side, price, quantity, now);
    scheduler_.schedule(deadline, [this, id, deadline] { activate(id, deadline); });
    return id;
  }
  void cancel(execution::OrderId id, market::Timestamp now) {
    advance(now);
    const auto deadline = std::max(after(now, latency_.cancel_ns), after(orders_.get(id).submitted, latency_.new_order_ns));
    if (!orders_.request_cancel(id, now)) return;
    scheduler_.schedule(deadline, [this, id, deadline] {
      if (orders_.acknowledge_cancel(id, deadline)) remove(id);
    });
  }
  void on_trade(const market::Trade& trade, market::Timestamp now, std::optional<market::Timestamp> occurred_ns = {}) {
    if (trade.price.ticks() <= 0 || trade.quantity <= 0) throw std::invalid_argument{"Invalid simulated trade"};
    if (occurred_ns && *occurred_ns > now) throw std::invalid_argument{"Trade occurs after observation"};
    advance(now);
    const auto side = trade.aggressor_side == market::AggressorSide::Sell ? market::Side::Bid : market::Side::Ask;
    const auto it = queues_.find({side, trade.price});
    if (it == queues_.end()) return;
    auto& queue = it->second;
    auto volume = trade.quantity;
    while (volume > 0 && !queue.empty()) {
      auto& entry = queue.front();
      if (entry.entered_ns > occurred_ns.value_or(now)) break;
      if (entry.id && !orders_.get(*entry.id).executable()) { queue.pop_front(); continue; }
      const auto consumed = std::min(volume, entry.quantity);
      if (entry.id) {
        const auto& order = orders_.get(*entry.id);
        orders_.fill(*entry.id, consumed, now);
        fills_.push_back({*entry.id, order.side, order.price, consumed, now});
      }
      volume -= consumed; entry.quantity -= consumed;
      if (entry.quantity == 0) queue.pop_front();
    }
    if (queue.empty()) queues_.erase(it);
  }
  void invalidate(market::Timestamp now) {
    advance(now);
    for (const auto& [id, order] : orders_.orders()) {
      if (order.terminal()) continue;
      if (order.state == execution::OrderState::PendingNew) orders_.reject(id, now);
      else { orders_.request_cancel(id, now); orders_.acknowledge_cancel(id, now); }
    }
    queues_.clear();
  }
  [[nodiscard]] const execution::OrderManager& orders() const noexcept { return orders_; }
  [[nodiscard]] const std::vector<Fill>& fills() const noexcept { return fills_; }
  [[nodiscard]] market::Timestamp now() const noexcept { return scheduler_.now(); }
  [[nodiscard]] market::Quantity queue_ahead(execution::OrderId id) const {
    const auto& order = orders_.get(id);
    const auto it = queues_.find({order.side, order.price});
    if (it == queues_.end()) return 0;
    market::Quantity ahead = 0;
    for (const auto& entry : it->second) {
      if (entry.id == id) return ahead;
      if (entry.quantity > std::numeric_limits<market::Quantity>::max() - ahead)
        throw std::out_of_range{"Queue quantity overflows"};
      ahead += entry.quantity;
    }
    return 0;
  }
private:
  struct Entry { std::optional<execution::OrderId> id; market::Quantity quantity; market::Timestamp entered_ns; };
  void activate(execution::OrderId id, market::Timestamp now) {
    const auto& order = orders_.get(id);
    if (order.terminal()) return;
    const auto bid = book_.best_bid(), ask = book_.best_ask();
    if (!bid || !ask || bid->price >= ask->price ||
        (order.side == market::Side::Bid && order.price >= ask->price) ||
        (order.side == market::Side::Ask && order.price <= bid->price)) {
      orders_.reject(id, now); return;
    }
    auto& queue = queues_[{order.side, order.price}];
    market::Quantity external = 0;
    for (const auto& entry : queue) if (!entry.id) external += entry.quantity;
    const auto displayed = book_.quantity_at(order.side, order.price).value_or(0);
    if (displayed > external) queue.push_back({std::nullopt, displayed - external, now});
    queue.push_back({id, order.remaining(), now}); orders_.accept(id, now);
  }
  void remove(execution::OrderId id) {
    const auto& order = orders_.get(id);
    const auto it = queues_.find({order.side, order.price});
    if (it == queues_.end()) return;
    std::erase_if(it->second, [id](const Entry& entry) { return entry.id == id; });
    if (it->second.empty()) queues_.erase(it);
  }
  const book::OrderBook& book_;
  Latency latency_;
  execution::OrderManager orders_;
  Scheduler scheduler_;
  std::map<std::pair<market::Side, market::Price>, std::deque<Entry>> queues_;
  std::vector<Fill> fills_;
};
} // namespace mercury::simulation
