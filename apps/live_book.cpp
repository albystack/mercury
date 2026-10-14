#include <mercury/app/arguments.hpp>
#include <mercury/feed/coinbase_parser.hpp>
#include <mercury/feed/feed_session.hpp>
#include <mercury/feed/live_feed.hpp>
#include <iostream>
int main(int argc, char** argv) {
  try {
    if (argc > 2) throw std::invalid_argument{"Usage: mercury_live [message_count]"};
    const auto count = argc == 2 ? mercury::app::positive_integer(argv[1]) : 50;
    mercury::feed::CoinbaseParser parser;
    mercury::feed::FeedSession session;
    std::size_t trades = 0;
    mercury::feed::run_live_feed(parser.instrument().product_id, count,
      [&](std::string_view raw, mercury::market::Timestamp received, bool new_connection) {
        if (new_connection || session.expire(received)) session.reset();
        if (session.consume(parser.parse(raw), received) == mercury::feed::SessionResult::Gap)
          throw std::runtime_error{"Sequence gap; resubscribing"};
        trades += session.trades().size();
      });
    if (!session.ready()) throw std::runtime_error{"No synchronized two-sided book received"};
    std::cout << "BTC-USD synchronized: bid=" << session.book().best_bid()->price.ticks()
              << " ask=" << session.book().best_ask()->price.ticks()
              << " (price ticks), live trades=" << trades << '\n';
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
