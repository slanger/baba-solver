// Code for formatting numbers.

#pragma once

#include <cstdint>
#include <string>

namespace BabaSolver {

// Formats the given number with a suffix, e.g. 10,000,000 -> "10M".
std::string FormatNumberWithSuffix(uint64_t n);

// Formats the given number with commas, e.g. 10000000 -> "10,000,000".
std::string FormatNumberWithCommas(uint64_t n);

}  // namespace BabaSolver
