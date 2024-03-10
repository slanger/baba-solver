// Code for solving Baba Is You levels.

#pragma once

#include <cstdint>
#include <memory>

#include "GameState.h"

namespace BabaSolver
{
	// Options to use when running the Baba Is You solver.
	// Use these options to trade off CPU usage, memory usage, and time to complete.
	struct SolverOptions
	{
		// The depth in the move tree at which algorithm switches from single-threaded to
		// multi-threaded. A higher value means a faster time to complete at the expense of more
		// CPU and memory usage.
		int parallelism_depth;
		// The max depth in the move tree at which to cache game states. A higher value trades CPU
		// usage for memory usage.
		int max_cache_depth;
		// How often (in number of moves) to print a debug log to stdout.
		uint64_t print_every_n_moves;

		// Initializes this object with reasonable defaults.
		SolverOptions() : parallelism_depth(2), max_cache_depth(20), print_every_n_moves(1'000'000) {}
	};

	// Solves the level given the initial state. See SolverOptions for options that can be tuned
	// for better performance.
	std::shared_ptr<GameState> Solve(const std::shared_ptr<GameState>& initial_state, const SolverOptions& options);

}  // namespace BabaSolver
