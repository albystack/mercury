#include <mercury/data/replay.hpp>
#include <fstream>
#include <iostream>
int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::invalid_argument{"Usage: mercury_replay recording.jsonl"};
    std::ifstream input{argv[1]};
    mercury::data::RecordingReader reader{input};
    mercury::feed::FeedSession session;
    const auto stats = mercury::data::replay(reader, session);
    if (!session.ready()) throw std::runtime_error{"Recording ends without a synchronized book"};
    std::cout << "records=" << stats.records << " updates=" << stats.book_updates
              << " trades=" << stats.trades << " gaps=" << stats.gaps
              << " bid=" << session.book().best_bid()->price.ticks()
              << " ask=" << session.book().best_ask()->price.ticks() << '\n';
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
