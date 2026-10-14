#pragma once
#include <mercury/feed/coinbase_websocket_client.hpp>
#include <mercury/market/types.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace mercury::feed {
[[nodiscard]] inline market::Timestamp steady_nanoseconds() {
  return static_cast<market::Timestamp>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
using MessageHandler = std::function<void(std::string_view, market::Timestamp, bool)>;
inline void run_live_feed(std::string_view product, std::size_t message_count, const MessageHandler& handler) {
  if (message_count == 0) throw std::invalid_argument{"Message count must be positive"};
  std::size_t received = 0;
  for (unsigned attempt = 0; attempt < 3; ++attempt) {
    try {
      CoinbaseWebSocketClient client;
      client.connect(); client.subscribe_level2(product);
      client.subscribe_market_trades(product); client.subscribe_heartbeats();
      bool first = true;
      while (received < message_count) {
        const auto raw = client.read_message();
        handler(raw, steady_nanoseconds(), first);
        first = false; ++received;
      }
      client.close(); return;
    } catch (const std::exception& error) {
      if (attempt == 2 || received == message_count) throw;
      std::cerr << "Feed reconnect: " << error.what() << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds{250 * (1 << attempt)});
    }
  }
}
} // namespace mercury::feed
