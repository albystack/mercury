#include <gtest/gtest.h>
#include <mercury/simulation/scheduler.hpp>
using namespace mercury;
TEST(Scheduler, DeadlineOrderAndEqualTimeFifo) {
  simulation::Scheduler scheduler; std::vector<int> calls;
  scheduler.schedule(20, [&] { calls.push_back(3); });
  scheduler.schedule(10, [&] { calls.push_back(1); });
  scheduler.schedule(10, [&] { calls.push_back(2); });
  scheduler.advance(9); EXPECT_TRUE(calls.empty());
  scheduler.advance(10); EXPECT_EQ(calls, (std::vector<int>{1, 2}));
  scheduler.advance(20); EXPECT_EQ(calls, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(scheduler.pending(), 0U);
}
TEST(Scheduler, RejectsClockRegressionAndOverflow) {
  simulation::Scheduler scheduler; scheduler.advance(10);
  EXPECT_THROW(scheduler.advance(9), std::invalid_argument);
  EXPECT_THROW(scheduler.schedule(9, [] {}), std::invalid_argument);
  EXPECT_THROW((void)simulation::after(std::numeric_limits<market::Timestamp>::max(), 1), std::out_of_range);
}
TEST(Scheduler, ActionsCanScheduleDeterministicFollowups) {
  simulation::Scheduler scheduler; int count = 0;
  scheduler.schedule(1, [&] { ++count; scheduler.schedule(2, [&] { ++count; }); });
  scheduler.advance(3); EXPECT_EQ(count, 2); EXPECT_EQ(scheduler.now(), 3U);
}
