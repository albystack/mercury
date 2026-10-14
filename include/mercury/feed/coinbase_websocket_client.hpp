#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <string>
#include <string_view>

namespace mercury::feed {

class CoinbaseWebSocketClient {
public:
  CoinbaseWebSocketClient();

  CoinbaseWebSocketClient(const CoinbaseWebSocketClient&) = delete;
  CoinbaseWebSocketClient& operator=(const CoinbaseWebSocketClient&) = delete;

  void connect();

  void subscribe_level2(std::string_view product_id);
  void subscribe_market_trades(std::string_view product_id);
  void subscribe_heartbeats();

  [[nodiscard]]
  std::string read_message();

  void close();

private:
  void subscribe(std::string_view channel, std::string_view product_id);
  using WebSocket = boost::beast::websocket::stream<
      boost::beast::ssl_stream<boost::beast::tcp_stream>>;

  boost::asio::io_context io_context_;
  boost::asio::ssl::context ssl_context_;
  boost::asio::ip::tcp::resolver resolver_;
  WebSocket websocket_;

  bool connected_{false};
  bool attempted_{false};
};

} // namespace mercury::feed
