#pragma once
#include <mercury/market/types.hpp>
#include <string_view>
namespace mercury::feed {
[[nodiscard]] std::int64_t decimal_units(std::string_view text, unsigned decimals);
[[nodiscard]] market::Timestamp utc_nanoseconds(std::string_view text);
} // namespace mercury::feed
