#pragma once
#include <mercury/feed/coinbase_parser.hpp>
#include <iosfwd>
#include <optional>
namespace mercury::data {
struct Record {
  market::Timestamp received_ns;
  bool new_connection;
  std::string raw;
};
class RecordingWriter {
public:
  RecordingWriter(std::ostream& output, const feed::Instrument& instrument);
  void write(const Record& record);
private:
  std::ostream& output_;
  std::optional<market::Timestamp> last_;
};
class RecordingReader {
public:
  explicit RecordingReader(std::istream& input);
  [[nodiscard]] const feed::Instrument& instrument() const noexcept { return instrument_; }
  [[nodiscard]] std::optional<Record> next();
private:
  std::istream& input_;
  feed::Instrument instrument_;
  std::size_t line_{1};
  std::optional<market::Timestamp> last_;
};
} // namespace mercury::data
