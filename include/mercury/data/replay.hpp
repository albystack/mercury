#pragma once
#include <mercury/data/recording.hpp>
#include <mercury/feed/feed_session.hpp>
#include <functional>
namespace mercury::data {
struct ReplayStats {
  std::size_t records{};
  std::size_t book_updates{};
  std::size_t trades{};
  std::size_t gaps{};
  std::size_t connections{};
};
using ReplayHandler = std::function<void(const Record&, const feed::FeedMessage&, const feed::FeedSession&)>;
inline ReplayStats replay(RecordingReader& reader, feed::FeedSession& session, const ReplayHandler& handler = {}, const ReplayHandler& before = {}) {
  feed::CoinbaseParser parser{reader.instrument()};
  ReplayStats stats;
  session.reset();
  while (const auto record = reader.next()) {
    const auto message = parser.parse(record->raw);
    if (before) before(*record, message, session);
    if (record->new_connection) { session.reset(); ++stats.connections; }
    session.expire(record->received_ns);
    const auto status = session.consume(message, record->received_ns);
    ++stats.records;
    if (status == feed::SessionResult::Gap) ++stats.gaps;
    if (status == feed::SessionResult::Applied) {
      for (const auto& batch : message.books) stats.book_updates += batch.updates.size();
      stats.trades += session.trades().size();
    }
    if (handler) handler(*record, message, session);
  }
  return stats;
}
} // namespace mercury::data
