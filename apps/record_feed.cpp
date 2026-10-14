#include <mercury/app/arguments.hpp>
#include <mercury/data/recording.hpp>
#include <mercury/feed/live_feed.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc, char** argv) {
  try {
    if (argc < 2 || argc > 3) throw std::invalid_argument{"Usage: mercury_record output.jsonl [message_count]"};
    if (std::filesystem::exists(argv[1])) throw std::invalid_argument{"Output already exists"};
    const auto count = argc == 3 ? mercury::app::positive_integer(argv[2]) : 1000;
    std::ofstream output{argv[1]};
    mercury::feed::Instrument instrument;
    mercury::data::RecordingWriter writer{output, instrument};
    mercury::feed::run_live_feed(instrument.product_id, count,
      [&](std::string_view raw, mercury::market::Timestamp received, bool first) {
        writer.write({received, first, std::string{raw}});
      });
    std::cout << "Recorded " << count << " messages to " << argv[1] << '\n';
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
