#include <mercury/feed/coinbase_websocket_client.hpp>

#include <exception>
#include <iostream>

int main()
{
  try {
    mercury::feed::CoinbaseWebSocketClient client;

    client.connect();
    client.subscribe_level2("BTC-USD");

    std::cout
        << "Connected to Coinbase Advanced Trade\n"
        << "Subscribed to BTC-USD level2\n\n";

    for (int message = 0; message < 5; ++message) {
      std::cout
          << client.read_message()
          << "\n\n";
    }

    client.close();
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
