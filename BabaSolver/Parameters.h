#pragma once

#include <cstdint>

namespace BabaSolver
{
	// Below are tunable parameters - use these to trade off CPU usage, memory usage, and time to complete.
	constexpr uint16_t MAX_TURN_DEPTH = 20;
	constexpr uint16_t PARALLELISM_DEPTH = 2;
	constexpr uint16_t CACHED_MOVES_MAX_TURN_DEPTH = 20;
	constexpr uint64_t PRINT_EVERY_N_MOVES = 1'000'000;
}
