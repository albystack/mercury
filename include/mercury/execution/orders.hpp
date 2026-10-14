#pragma once
#include <mercury/market/types.hpp>
#include <limits>
#include <map>
#include <stdexcept>
namespace mercury::execution {
using OrderId = std::uint64_t;
enum class OrderState { PendingNew, Live, PartiallyFilled, PendingCancel, Filled, Cancelled, Rejected };
struct Order {
  OrderId id;
  market::Side side;
  market::Price price;
  market::Quantity quantity;
  market::Quantity filled{0};
  OrderState state{OrderState::PendingNew};
  market::Timestamp submitted;
  market::Timestamp updated;
  bool cancel_requested{false};
  [[nodiscard]] market::Quantity remaining() const noexcept { return quantity - filled; }
  [[nodiscard]] bool terminal() const noexcept {
    return state == OrderState::Filled || state == OrderState::Cancelled || state == OrderState::Rejected;
  }
  [[nodiscard]] bool executable() const noexcept {
    return state == OrderState::Live || state == OrderState::PartiallyFilled || state == OrderState::PendingCancel;
  }
};
class OrderManager {
public:
  OrderId submit(market::Side side, market::Price price, market::Quantity quantity, market::Timestamp now) {
    if (price.ticks() <= 0 || quantity <= 0 || next_id_ == std::numeric_limits<OrderId>::max())
      throw std::invalid_argument{"Invalid order"};
    const auto id = next_id_;
    orders_.emplace(id, Order{id, side, price, quantity, 0, OrderState::PendingNew, now, now, false});
    ++next_id_; return id;
  }
  void accept(OrderId id, market::Timestamp now) {
    auto& order = mutable_order(id, now);
    if (order.state != OrderState::PendingNew) throw std::logic_error{"Order is not pending new"};
    order.state = order.cancel_requested ? OrderState::PendingCancel : OrderState::Live;
    order.updated = now;
  }
  void reject(OrderId id, market::Timestamp now) {
    auto& order = mutable_order(id, now);
    if (order.state != OrderState::PendingNew) throw std::logic_error{"Only pending orders may be rejected"};
    order.state = OrderState::Rejected; order.updated = now;
  }
  bool request_cancel(OrderId id, market::Timestamp now) {
    auto& order = mutable_order(id, now);
    if (order.terminal() || order.cancel_requested) return false;
    order.cancel_requested = true;
    if (order.executable()) order.state = OrderState::PendingCancel;
    order.updated = now; return true;
  }
  bool acknowledge_cancel(OrderId id, market::Timestamp now) {
    auto& order = mutable_order(id, now);
    if (order.terminal()) return false;
    if (!order.cancel_requested) throw std::logic_error{"Cancellation was not requested"};
    order.state = OrderState::Cancelled; order.updated = now; return true;
  }
  void fill(OrderId id, market::Quantity quantity, market::Timestamp now) {
    auto& order = mutable_order(id, now);
    if (!order.executable()) throw std::logic_error{"Order is not executable"};
    if (quantity <= 0 || quantity > order.remaining()) throw std::invalid_argument{"Invalid fill quantity"};
    order.filled += quantity;
    if (order.remaining() == 0) order.state = OrderState::Filled;
    else if (!order.cancel_requested) order.state = OrderState::PartiallyFilled;
    order.updated = now;
  }
  [[nodiscard]] const Order& get(OrderId id) const { return orders_.at(id); }
  [[nodiscard]] const std::map<OrderId, Order>& orders() const noexcept { return orders_; }
private:
  Order& mutable_order(OrderId id, market::Timestamp now) {
    auto& order = orders_.at(id);
    if (now < order.updated) throw std::invalid_argument{"Order clock moved backwards"};
    return order;
  }
  std::map<OrderId, Order> orders_;
  OrderId next_id_{1};
};
} // namespace mercury::execution
