#include <mercury/feed/coinbase_websocket_client.hpp>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core.hpp>

#include <openssl/ssl.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <chrono>
#include <string>

namespace mercury::feed {

namespace {

constexpr std::string_view kHost{
    "advanced-trade-ws.coinbase.com"
};

constexpr std::string_view kPort{"443"};
constexpr std::string_view kPath{"/"};

} // namespace


CoinbaseWebSocketClient::CoinbaseWebSocketClient()
    : ssl_context_{boost::asio::ssl::context::tls_client},
      resolver_{io_context_},
      websocket_{io_context_, ssl_context_}
{
  ssl_context_.set_default_verify_paths();

  websocket_.next_layer().set_verify_mode(
      boost::asio::ssl::verify_peer);

  websocket_.next_layer().set_verify_callback(
      boost::asio::ssl::host_name_verification{
          std::string{kHost}
      });
}


void CoinbaseWebSocketClient::connect()
{
  if (attempted_) {
    throw std::logic_error{
        "Create a fresh client for each connection attempt"
    };
  }

  attempted_ = true;
  boost::beast::get_lowest_layer(websocket_).expires_after(std::chrono::seconds{10});
  if (!SSL_set_tlsext_host_name(
          websocket_.next_layer().native_handle(),
          kHost.data())) {
    throw std::runtime_error{
        "Failed to set TLS SNI hostname"
    };
  }

  const auto endpoints = resolver_.resolve(
      std::string{kHost},
      std::string{kPort});

  boost::beast::get_lowest_layer(websocket_).connect(
      endpoints);

  websocket_.next_layer().handshake(
      boost::asio::ssl::stream_base::client);

  boost::beast::get_lowest_layer(websocket_).expires_never();
  auto timeout = boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client);
  timeout.handshake_timeout = std::chrono::seconds{10};
  timeout.idle_timeout = std::chrono::seconds{10};
  timeout.keep_alive_pings = true;
  websocket_.set_option(timeout);
  websocket_.read_message_max(32 * 1024 * 1024);
  websocket_.handshake(
      std::string{kHost},
      std::string{kPath});

  connected_ = true;
}


void CoinbaseWebSocketClient::subscribe_level2(std::string_view product_id) {
  subscribe("level2", product_id);
}
void CoinbaseWebSocketClient::subscribe_market_trades(std::string_view product_id) {
  subscribe("market_trades", product_id);
}
void CoinbaseWebSocketClient::subscribe_heartbeats() {
  subscribe("heartbeats", {});
}
void CoinbaseWebSocketClient::subscribe(std::string_view channel, std::string_view product_id) {
  if (!connected_) throw std::logic_error{"Cannot subscribe before connecting"};
  if (channel != "heartbeats" && product_id.empty()) throw std::invalid_argument{"Empty product ID"};
  nlohmann::json products = nlohmann::json::array();
  if (!product_id.empty()) products.push_back(product_id);
  const auto subscription = nlohmann::json{{"type", "subscribe"}, {"channel", channel},
                                          {"product_ids", products}}.dump();
  websocket_.write(boost::asio::buffer(subscription));
}

std::string CoinbaseWebSocketClient::read_message()
{
  if (!connected_) {
    throw std::logic_error{
        "Cannot read before connecting"
    };
  }

  boost::beast::flat_buffer buffer;

  boost::system::error_code error;
  io_context_.restart();
  websocket_.async_read(buffer, [&](boost::system::error_code ec, std::size_t) { error = ec; });
  io_context_.run();
  if (error) { connected_ = false; throw boost::system::system_error{error}; }

  return boost::beast::buffers_to_string(
      buffer.data());
}


void CoinbaseWebSocketClient::close()
{
  if (!connected_) {
    return;
  }

  boost::system::error_code error;
  io_context_.restart();
  websocket_.async_close(boost::beast::websocket::close_code::normal,
                        [&](boost::system::error_code ec) { error = ec; });
  io_context_.run();
  connected_ = false;
  if (error) throw boost::system::system_error{error};
}

} // namespace mercury::feed
