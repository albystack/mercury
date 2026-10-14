#pragma once
#include <mercury/book/order_book.hpp>
#include <mercury/feed/coinbase_parser.hpp>
#include <deque>
#include <optional>
#include <unordered_set>
namespace mercury::feed {
enum class SessionResult { Applied, Ignored, Gap };
class FeedSession {
public:
  explicit FeedSession(market::Timestamp stale_after_ns = 10'000'000'000ULL)
      : stale_after_ns_{stale_after_ns} {
    if (stale_after_ns == 0) throw std::invalid_argument{"Zero stale threshold"};
  }
  void reset() noexcept {
    book_ = {}; sequence_.reset(); last_received_.reset(); synchronized_ = false;
    recent_ids_.clear(); ids_.clear(); trades_.clear();
  }
  SessionResult consume(const FeedMessage& message, market::Timestamp received_ns) {
    trades_.clear();
    if (last_received_ && received_ns < *last_received_)
      throw std::invalid_argument{"Arrival clock moved backwards"};
    if (sequence_ && message.sequence <= *sequence_) return SessionResult::Ignored;
    if (sequence_ && (message.sequence - *sequence_ != 1)) {
      reset(); sequence_ = message.sequence; last_received_ = received_ns;
      return SessionResult::Gap;
    }
    sequence_ = message.sequence; last_received_ = received_ns;
    for (const auto& batch : message.books) {
      if (!batch.snapshot && !synchronized_) continue;
      if (batch.snapshot) { book_ = {}; synchronized_ = true; }
      for (const auto& update : batch.updates)
        book_.update(update.side, update.price, update.quantity);
      const auto bid = book_.best_bid(), ask = book_.best_ask();
      if (bid && ask && bid->price >= ask->price) {
        reset(); throw std::invalid_argument{"Crossed feed book; fresh snapshot required"};
      }
    }
    for (const auto& batch : message.trades) {
      for (const auto& value : batch.trades) {
        if (ids_.contains(value.trade_id)) continue;
        ids_.insert(value.trade_id); recent_ids_.push_back(value.trade_id);
        if (recent_ids_.size() > 10'000) { ids_.erase(recent_ids_.front()); recent_ids_.pop_front(); }
        if (!batch.snapshot && synchronized_) trades_.push_back(value.trade);
      }
    }
    return SessionResult::Applied;
  }
  bool expire(market::Timestamp now_ns) {
    if (last_received_ && now_ns >= *last_received_ && now_ns - *last_received_ > stale_after_ns_) {
      reset(); return true;
    }
    return false;
  }
  [[nodiscard]] std::optional<std::uint64_t> last_sequence() const noexcept { return sequence_; }
  [[nodiscard]] bool synchronized() const noexcept { return synchronized_; }
  [[nodiscard]] bool ready() const {
    return synchronized_ && book_.best_bid().has_value() && book_.best_ask().has_value();
  }
  [[nodiscard]] const book::OrderBook& book() const noexcept { return book_; }
  [[nodiscard]] const std::vector<market::Trade>& trades() const noexcept { return trades_; }
private:
  book::OrderBook book_;
  std::optional<std::uint64_t> sequence_;
  std::optional<market::Timestamp> last_received_;
  market::Timestamp stale_after_ns_;
  bool synchronized_{false};
  std::deque<std::string> recent_ids_;
  std::unordered_set<std::string> ids_;
  std::vector<market::Trade> trades_;
};
} // namespace mercury::feed
