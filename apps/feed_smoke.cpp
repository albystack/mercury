#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/ssl.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace websocket = beast::websocket;

using tcp = net::ip::tcp;

int main()
{
  try {
    const std::string host{"advanced-trade-ws.coinbase.com"};
    const std::string port{"443"};

    net::io_context io_context;

    ssl::context ssl_context{ssl::context::tls_client};
    ssl_context.set_default_verify_paths();

    tcp::resolver resolver{io_context};

    websocket::stream<beast::ssl_stream<beast::tcp_stream>> websocket{
        io_context,
        ssl_context
    };

    websocket.next_layer().set_verify_mode(ssl::verify_peer);
    websocket.next_layer().set_verify_callback(
        ssl::host_name_verification{host});

    // TLS requires the hostname to be supplied through SNI.
    if (!SSL_set_tlsext_host_name(
            websocket.next_layer().native_handle(),
            host.c_str())) {
      throw std::runtime_error{"Failed to set TLS SNI hostname"};
    }

    const auto endpoints = resolver.resolve(host, port);

    beast::get_lowest_layer(websocket).connect(endpoints);

    websocket.next_layer().handshake(
        ssl::stream_base::client);

    websocket.handshake(host, "/");

    const std::string subscription{
        R"({"type":"subscribe","channel":"level2","product_ids":["BTC-USD"]})"
    };

    websocket.write(net::buffer(subscription));

    std::cout << "Connected to Coinbase Advanced Trade\n";
    std::cout << "Subscribed to BTC-USD level2\n\n";

    for (int message = 0; message < 5; ++message) {
      beast::flat_buffer buffer;

      websocket.read(buffer);

      std::cout
          << beast::buffers_to_string(buffer.data())
          << "\n\n";
    }

    websocket.close(websocket::close_code::normal);
  }
  catch (const std::exception& exception) {
    std::cerr
        << "Feed smoke test failed: "
        << exception.what()
        << '\n';

    return 1;
  }

  return 0;
}