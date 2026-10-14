#include <mercury/app/arguments.hpp>
#include <mercury/book/order_book.hpp>
#include <mercury/feed/coinbase_parser.hpp>
#include <mercury/feed/live_feed.hpp>
#include <mercury/metrics/latency.hpp>
#include <iostream>
namespace {
void report(const char* name, const mercury::metrics::LatencySamples& samples, mercury::market::Timestamp elapsed) {
  const auto s = samples.summary();
  std::cout << name << ": samples=" << s.count << " median_ns=" << s.median_ns << " p95_ns=" << s.p95_ns
            << " p99_ns=" << s.p99_ns << " mean_ns=" << s.mean_ns
            << " operations_per_second=" << (elapsed == 0 ? 0 : static_cast<long double>(s.count) * 1e9L / static_cast<long double>(elapsed)) << '\n';
}
}
int main(int argc, char** argv) {
  try {
    if (argc > 2) throw std::invalid_argument{"Usage: mercury_benchmark [samples]"};
    const auto count = argc == 2 ? mercury::app::positive_integer(argv[1]) : 10000;
    mercury::book::OrderBook book;
    mercury::metrics::LatencySamples updates, parses;
    const auto start = mercury::feed::steady_nanoseconds();
    for (std::uint64_t i = 0; i < count; ++i) {
      const auto before = mercury::feed::steady_nanoseconds();
      book.update(mercury::market::Side::Bid, mercury::market::Price{10000 + static_cast<std::int64_t>(i % 1000)},
                  static_cast<mercury::market::Quantity>(i % 100 + 1));
      updates.add(mercury::feed::steady_nanoseconds() - before);
    }
    const auto end = mercury::feed::steady_nanoseconds();
    mercury::feed::CoinbaseParser parser;
    const std::string text = R"({"channel":"l2_data","sequence_num":0,"timestamp":"2026-10-14T12:00:00Z","events":[{"type":"update","product_id":"BTC-USD","updates":[{"side":"bid","price_level":"100.00","new_quantity":"0.001","event_time":"2026-10-14T12:00:00Z"}]}]})";
    std::size_t checksum = 0;
    const auto parse_start = mercury::feed::steady_nanoseconds();
    for (std::uint64_t i = 0; i < count; ++i) {
      const auto before = mercury::feed::steady_nanoseconds();
      const auto message = parser.parse(text); checksum += message.books[0].updates.size();
      parses.add(mercury::feed::steady_nanoseconds() - before);
    }
    report("book_update", updates, end - start);
    report("level2_parse", parses, mercury::feed::steady_nanoseconds() - parse_start);
    std::cout << "checksum=" << checksum << " best_bid=" << book.best_bid()->price.ticks() << '\n';
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
