#include "NumberFormat.h"

#include <cstdint>
#include <string>

namespace BabaSolver {

std::string FormatNumberWithSuffix(uint64_t n) {
  if (n >= 1'000'000'000) return std::to_string(n / 1'000'000'000) + "B";
  if (n >= 1'000'000) return std::to_string(n / 1'000'000) + "M";
  if (n >= 1'000) return std::to_string(n / 1'000) + "K";
  return std::to_string(n);
}

std::string FormatNumberWithCommas(uint64_t n) {
  std::string s = std::to_string(n);
  int64_t size = s.length() - 3;
  while (size > 0) {
    s.insert(size, ",");
    size -= 3;
  }
  return s;
}

}  // namespace BabaSolver
