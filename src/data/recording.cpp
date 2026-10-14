#include <mercury/data/recording.hpp>
#include <nlohmann/json.hpp>
#include <istream>
#include <ostream>
#include <stdexcept>
namespace mercury::data {
namespace {
std::uint64_t unsigned_field(const nlohmann::json& j, const char* key) {
  const auto& value = j.at(key);
  if (!value.is_number_unsigned()) throw std::invalid_argument{"Expected unsigned integer"};
  return value.get<std::uint64_t>();
}
void validate_record(const Record& record, std::optional<market::Timestamp> last) {
  if (record.raw.empty() || (!last && !record.new_connection) || (last && record.received_ns < *last))
    throw std::invalid_argument{"Invalid record or nonmonotonic arrival time"};
}
}
RecordingWriter::RecordingWriter(std::ostream& output, const feed::Instrument& instrument) : output_{output} {
  const feed::CoinbaseParser validate{instrument};
  output_ << nlohmann::json{{"version", 1}, {"product_id", instrument.product_id},
      {"price_decimals", instrument.price_decimals}, {"quantity_decimals", instrument.quantity_decimals}}.dump() << '\n';
  if (!output_) throw std::runtime_error{"Cannot write recording header"};
}
void RecordingWriter::write(const Record& record) {
  validate_record(record, last_);
  output_ << nlohmann::json{{"received_ns", record.received_ns}, {"new_connection", record.new_connection},
                           {"raw", record.raw}}.dump() << '\n';
  output_.flush();
  if (!output_) throw std::runtime_error{"Cannot write recording"};
  last_ = record.received_ns;
}
RecordingReader::RecordingReader(std::istream& input) : input_{input} {
  std::string line;
  if (!std::getline(input_, line)) throw std::invalid_argument{"Missing recording header"};
  try {
    const auto j = nlohmann::json::parse(line);
    if (unsigned_field(j, "version") != 1) throw std::invalid_argument{"Unsupported recording version"};
    const auto prices = unsigned_field(j, "price_decimals"), quantities = unsigned_field(j, "quantity_decimals");
    if (prices > 9 || quantities > 9) throw std::invalid_argument{"Unsupported instrument scale"};
    instrument_ = {j.at("product_id").get<std::string>(), static_cast<unsigned>(prices), static_cast<unsigned>(quantities)};
    const feed::CoinbaseParser validate{instrument_};
  } catch (const nlohmann::json::exception& error) {
    throw std::invalid_argument{std::string{"Invalid recording header: "} + error.what()};
  }
}
std::optional<Record> RecordingReader::next() {
  std::string line;
  if (!std::getline(input_, line)) {
    if (input_.bad()) throw std::runtime_error{"Recording read failure"};
    return std::nullopt;
  }
  ++line_;
  try {
    const auto j = nlohmann::json::parse(line);
    Record record{unsigned_field(j, "received_ns"), j.at("new_connection").get<bool>(), j.at("raw").get<std::string>()};
    validate_record(record, last_); last_ = record.received_ns; return record;
  } catch (const std::exception& error) {
    throw std::invalid_argument{"Recording line " + std::to_string(line_) + ": " + error.what()};
  }
}
} // namespace mercury::data
