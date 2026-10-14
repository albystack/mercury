#pragma once
#include <mercury/data/replay.hpp>
#include <mercury/risk/limits.hpp>
#include <mercury/strategy/quotes.hpp>
namespace mercury::simulation {
struct PaperConfig {
  Latency latency{};
  strategy::QuoteConfig quotes{};
  risk::Limits limits{};
  long double fee_bps{0};
};
class PaperEngine {
public:
  PaperEngine(const feed::FeedSession& session, const feed::Instrument& instrument, PaperConfig config = {})
      : exchange_{session.book(), config.latency}, account_{instrument, config.fee_bps},
        risk_{config.limits}, quoter_{config.quotes} {
    if (config.quotes.quantity > config.limits.max_order_quantity ||
        config.quotes.inventory_limit > config.limits.max_position)
      throw std::invalid_argument{"Quote sizes exceed risk configuration"};
  }
  void before(const data::Record& record, const feed::FeedMessage& message, const feed::FeedSession& session) {
    if (record.new_connection || !received_anchor_) {
      received_anchor_ = record.received_ns; exchange_anchor_ = message.timestamp;
    }
    const auto previous = session.last_sequence();
    const bool fresh = !previous || message.sequence > *previous;
    const bool gap = previous && fresh && message.sequence - *previous != 1;
    const bool snapshot = fresh && std::any_of(message.books.begin(), message.books.end(),
                                             [](const feed::BookBatch& batch) { return batch.snapshot; });
    if (record.new_connection || gap || snapshot) {
      exchange_.invalidate(record.received_ns); bid_.reset(); ask_.reset();
    } else exchange_.advance(record.received_ns);
  }
  void observe(const data::Record& record, const feed::FeedSession& session) {
    const auto now = record.received_ns;
    if (!session.ready()) {
      exchange_.invalidate(now); bid_.reset(); ask_.reset(); return;
    }
    for (const auto& trade : session.trades()) {
      exchange_.on_trade(trade, now, trade_clock(trade.timestamp, now));
      while (accounted_fills_ < exchange_.fills().size()) account_.apply(exchange_.fills()[accounted_fills_++]);
      const auto absolute = account_.inventory() < 0 ? -account_.inventory() : account_.inventory();
      max_inventory_ = std::max(max_inventory_, absolute);
    }
    const auto bid = session.book().best_bid()->price.ticks(), ask = session.book().best_ask()->price.ticks();
    mark_ = market::Price{bid + (ask - bid) / 2}; risk_.observe(account_.equity(*mark_));
    const auto desired = risk_.halted() ? strategy::Quotes{} : quoter_.quotes(session.book(), account_.inventory());
    reconcile(market::Side::Bid, desired.bid, bid_, now);
    reconcile(market::Side::Ask, desired.ask, ask_, now);
  }
  void kill(market::Timestamp now) {
    risk_.kill(); reconcile(market::Side::Bid, {}, bid_, now); reconcile(market::Side::Ask, {}, ask_, now);
  }
  void finish() { exchange_.invalidate(exchange_.now()); bid_.reset(); ask_.reset(); }
  [[nodiscard]] const risk::Account& account() const noexcept { return account_; }
  [[nodiscard]] const risk::RiskManager& risk_manager() const noexcept { return risk_; }
  [[nodiscard]] const SimulatedExchange& exchange() const noexcept { return exchange_; }
  [[nodiscard]] std::size_t submitted() const noexcept { return submitted_; }
  [[nodiscard]] std::size_t cancellations() const noexcept { return cancellations_; }
  [[nodiscard]] std::size_t risk_rejections() const noexcept { return risk_rejections_; }
  [[nodiscard]] market::Quantity max_inventory() const noexcept { return max_inventory_; }
  [[nodiscard]] long double equity() const noexcept { return mark_ ? account_.equity(*mark_) : account_.cash(); }
private:
  [[nodiscard]] market::Timestamp trade_clock(market::Timestamp exchange_time, market::Timestamp received) const {
    if (!received_anchor_ || !exchange_anchor_) throw std::logic_error{"Paper engine requires before callback"};
    market::Timestamp mapped;
    if (exchange_time >= *exchange_anchor_) mapped = after(*received_anchor_, exchange_time - *exchange_anchor_);
    else {
      const auto difference = *exchange_anchor_ - exchange_time;
      mapped = difference > *received_anchor_ ? 0 : *received_anchor_ - difference;
    }
    return std::min(mapped, received);
  }
  void reconcile(market::Side side, const std::optional<strategy::Quote>& desired,
                 std::optional<execution::OrderId>& active, market::Timestamp now) {
    exchange_.advance(now);
    if (active && exchange_.orders().get(*active).terminal()) active.reset();
    if (active) {
      const auto& order = exchange_.orders().get(*active);
      if (desired && order.price == desired->price && order.quantity == desired->quantity && !order.cancel_requested) return;
      if (!order.cancel_requested) { exchange_.cancel(*active, now); ++cancellations_; }
      return;
    }
    if (!desired) return;
    if (!risk_.allow(side, desired->price, desired->quantity, account_, exchange_.orders(), true)) {
      ++risk_rejections_; return;
    }
    active = exchange_.submit(side, desired->price, desired->quantity, now); ++submitted_;
  }
  SimulatedExchange exchange_;
  risk::Account account_;
  risk::RiskManager risk_;
  strategy::InventoryQuoter quoter_;
  std::optional<execution::OrderId> bid_, ask_;
  std::optional<market::Price> mark_;
  std::optional<market::Timestamp> received_anchor_, exchange_anchor_;
  std::size_t accounted_fills_{0}, submitted_{0}, cancellations_{0}, risk_rejections_{0};
  market::Quantity max_inventory_{0};
};
} // namespace mercury::simulation
