#pragma once
#include <mercury/market/types.hpp>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <vector>
namespace mercury::simulation {
[[nodiscard]] inline market::Timestamp after(market::Timestamp now, market::Timestamp delay) {
  if (delay > std::numeric_limits<market::Timestamp>::max() - now)
    throw std::out_of_range{"Latency timestamp overflows"};
  return now + delay;
}
struct Latency {
  market::Timestamp new_order_ns{1'000'000};
  market::Timestamp cancel_ns{1'000'000};
};
class Scheduler {
public:
  Scheduler() = default;
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
  void schedule(market::Timestamp deadline, std::function<void()> action) {
    if (deadline < now_ || !action || next_sequence_ == std::numeric_limits<std::uint64_t>::max())
      throw std::invalid_argument{"Invalid scheduled action"};
    actions_.push({deadline, next_sequence_, std::move(action)}); ++next_sequence_;
  }
  void advance(market::Timestamp target) {
    if (target < now_) throw std::invalid_argument{"Simulation clock moved backwards"};
    while (!actions_.empty() && actions_.top().deadline <= target) {
      auto action = actions_.top(); actions_.pop(); now_ = action.deadline;
      action.callback();
    }
    now_ = target;
  }
  [[nodiscard]] market::Timestamp now() const noexcept { return now_; }
  [[nodiscard]] std::size_t pending() const noexcept { return actions_.size(); }
private:
  struct Action {
    market::Timestamp deadline;
    std::uint64_t sequence;
    std::function<void()> callback;
  };
  struct Later {
    bool operator()(const Action& left, const Action& right) const noexcept {
      return left.deadline != right.deadline ? left.deadline > right.deadline : left.sequence > right.sequence;
    }
  };
  std::priority_queue<Action, std::vector<Action>, Later> actions_;
  market::Timestamp now_{0};
  std::uint64_t next_sequence_{0};
};
} // namespace mercury::simulation
