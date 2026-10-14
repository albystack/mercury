#pragma once
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string_view>
namespace mercury::app {
[[nodiscard]] inline std::uint64_t positive_integer(std::string_view text) {
  std::uint64_t value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size() || value == 0 || value > 1'000'000)
    throw std::invalid_argument{"Expected integer in [1, 1000000]"};
  return value;
}
} // namespace mercury::app
