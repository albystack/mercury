#include <mercury/app/arguments.hpp>
#include <mercury/simulation/paper_engine.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc, char** argv) {
  try {
    if (argc < 2 || argc > 5)
      throw std::invalid_argument{"Usage: mercury_backtest recording.jsonl [latency_us] [fee_bps] [order_units]"};
    std::ifstream input{argv[1]}; mercury::data::RecordingReader reader{input};
    mercury::feed::FeedSession session;
    mercury::simulation::PaperConfig config;
    if (argc > 2) config.latency = {mercury::app::integer(argv[2]) * 1000, mercury::app::integer(argv[2]) * 1000};
    if (argc > 3) config.fee_bps = static_cast<long double>(mercury::app::integer(argv[3]));
    if (argc > 4) {
      const auto units = static_cast<mercury::market::Quantity>(mercury::app::positive_integer(argv[4]));
      config.quotes.quantity = units; config.quotes.inventory_limit = units * 10;
      config.limits.max_order_quantity = units; config.limits.max_position = units * 10;
    }
    mercury::simulation::PaperEngine engine{session, reader.instrument(), config};
    const auto stats = mercury::data::replay(reader, session,
      [&](const auto& record, const auto&, const auto& state) { engine.observe(record, state); },
      [&](const auto& record, const auto& message, const auto& state) { engine.before(record, message, state); });
    if (!session.ready()) throw std::runtime_error{"Recording ends without a synchronized book"};
    engine.finish();
    std::cout << std::fixed << std::setprecision(10)
              << "records=" << stats.records << " gaps=" << stats.gaps << " connections=" << stats.connections
              << " orders=" << engine.submitted() << " fills=" << engine.account().fill_count()
              << " cancels=" << engine.cancellations() << " risk_rejections=" << engine.risk_rejections()
              << " inventory_units=" << engine.account().inventory() << " max_inventory_units=" << engine.max_inventory()
              << " cash=" << engine.account().cash() << " equity=" << engine.equity()
              << " fees=" << engine.account().fees() << " turnover=" << engine.account().turnover()
              << " max_drawdown=" << engine.risk_manager().max_drawdown()
              << " halted=" << engine.risk_manager().halted() << '\n';
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
