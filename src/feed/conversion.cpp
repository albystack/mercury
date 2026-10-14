#include <mercury/feed/conversion.hpp>
#include <chrono>
#include <limits>
#include <stdexcept>
namespace mercury::feed {
std::int64_t decimal_units(std::string_view text, unsigned decimals) {
  if (text.empty() || decimals > 9) throw std::invalid_argument{"Invalid decimal scale or value"};
  std::int64_t value = 0;
  bool dot = false;
  unsigned fractional = 0;
  unsigned digits = 0;
  for (const char c : text) {
    if (c == '.' && !dot && digits > 0) { dot = true; continue; }
    if (c < '0' || c > '9') throw std::invalid_argument{"Expected unsigned decimal text"};
    ++digits;
    if (dot && ++fractional > decimals) {
      if (c != '0') throw std::invalid_argument{"Value is off the configured increment"};
      continue;
    }
    const auto digit = static_cast<std::int64_t>(c - '0');
    if (value > (std::numeric_limits<std::int64_t>::max() - digit) / 10)
      throw std::out_of_range{"Decimal value overflows"};
    value = value * 10 + digit;
  }
  if (digits == 0 || (dot && fractional == 0)) throw std::invalid_argument{"Incomplete decimal"};
  for (unsigned i = dot ? fractional : 0; i < decimals; ++i) {
    if (value > std::numeric_limits<std::int64_t>::max() / 10)
      throw std::out_of_range{"Scaled decimal overflows"};
    value *= 10;
  }
  return value;
}
market::Timestamp utc_nanoseconds(std::string_view text) {
  using namespace std::chrono;
  if (text.size() < 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
      text[13] != ':' || text[16] != ':' || text.back() != 'Z')
    throw std::invalid_argument{"Expected UTC ISO-8601 timestamp"};
  const auto number = [&](std::size_t start, std::size_t count) {
    int value = 0;
    for (const char c : text.substr(start, count)) {
      if (c < '0' || c > '9') throw std::invalid_argument{"Invalid timestamp digits"};
      value = value * 10 + (c - '0');
    }
    return value;
  };
  const year_month_day date{year{number(0, 4)}, month{static_cast<unsigned>(number(5, 2))},
                            day{static_cast<unsigned>(number(8, 2))}};
  const int hour = number(11, 2), minute = number(14, 2), second = number(17, 2);
  if (!date.ok() || int(date.year()) < 1970 || int(date.year()) > 2261 ||
      hour > 23 || minute > 59 || second > 59)
    throw std::invalid_argument{"Timestamp outside supported range"};
  std::int64_t fraction = 0;
  if (text.size() != 20) {
    if (text[19] != '.' || text.size() < 22 || text.size() > 30)
      throw std::invalid_argument{"Invalid timestamp precision"};
    fraction = decimal_units(std::string{"0"} + std::string{text.substr(19, text.size() - 20)}, 9);
  }
  const auto elapsed = sys_days{date}.time_since_epoch() + hours{hour} + minutes{minute} + seconds{second};
  return static_cast<market::Timestamp>(duration_cast<nanoseconds>(elapsed).count() + fraction);
}
} // namespace mercury::feed
