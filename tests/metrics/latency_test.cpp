#include <gtest/gtest.h>
#include <mercury/metrics/latency.hpp>
TEST(LatencySamples, KnownDistributionAndEmptySample) {
  mercury::metrics::LatencySamples samples; EXPECT_EQ(samples.summary().count, 0U);
  for (std::uint64_t i = 100; i > 0; --i) samples.add(i);
  const auto s = samples.summary(); EXPECT_EQ(s.min_ns, 1U); EXPECT_EQ(s.max_ns, 100U);
  EXPECT_EQ(s.median_ns, 50U); EXPECT_EQ(s.p95_ns, 95U); EXPECT_EQ(s.p99_ns, 99U);
  EXPECT_DOUBLE_EQ(static_cast<double>(s.mean_ns), 50.5);
}
TEST(LatencySamples, SmallSamplesUseNearestRank) {
  mercury::metrics::LatencySamples samples; samples.add(7); samples.add(1); samples.add(3);
  const auto s = samples.summary(); EXPECT_EQ(s.median_ns, 3U); EXPECT_EQ(s.p99_ns, 7U);
}
