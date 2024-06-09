// Code for solving Baba Is You levels.
//
// This solver uses a brute force algorithm which calculates the tree of all possible moves and
// returns the leaf state that has the best score (which may or may not be a winning state).
// Various optimizations and heuristics are performed to prune paths of the tree that would lead to
// game states that are impossible/very unlikely to win.
//
// Note: There's no guarantee that the solution this algorithm finds is the most optimal solution
// (i.e. the solution with the least number of moves). 
//
// Glossary:
// * Move: Represents an input that a player can make (i.e. up, down, left, or right).
// * Game state: The state of the game, including the states of all game objects.
// * Move tree: The tree of possible moves starting with an initial game state. Game states are the
//     nodes of the tree and moves are the edges. Since there are 4 possible moves for each game
//     state, the tree grows at a rate of 4^n.
// * Turn count: The depth of the move tree at a given game state.
// * Game state score: Represents how likely the game state will lead to a winning game state. This
//     score generally depends on heuristics and is level specific.

#pragma once

#include <cstdint>
#include <memory>

#include "GameState.h"

namespace BabaSolver
{
	// Options to use when running the Baba Is You solver.
	// Use these options to trade off CPU usage, memory usage, thread usage, and time to complete.
	struct SolverOptions
	{
		// How many iterations to run the solver.
		int iteration_count;
		// The max depth in the move tree the algorithm will go in one iteration. The number of
		// moves calculated grows exponentially with this value.
		int max_turn_depth;
		// The depth in the move tree at which the algorithm switches from single-threaded to
		// multi-threaded. A higher value means higher parallelism (up to the limits of the
		// computer's CPU), which generally leads to a faster time to complete at the expense of
		// more CPU and memory usage.
		int parallelism_depth;
		// The max depth in the move tree at which to cache game states. A higher value trades CPU
		// usage for memory usage.
		int max_cache_depth;
		// How often (in number of moves) to print a debug log to stdout.
		uint64_t print_every_n_moves;

		// Initializes this object with reasonable defaults.
		SolverOptions() : iteration_count(4), max_turn_depth(25), parallelism_depth(2), max_cache_depth(20), print_every_n_moves(10'000'000) {}
	};

	// Tries to solve the level given the initial state and options. Returns the winning game state
	// if achieveable with the given options, otherwise returns the game state with the best score
	// at the end of the last iteration. The score is determined by GameState::CalculateScore().
	// See SolverOptions for options that can be tuned for better performance.
	std::shared_ptr<GameState> Solve(const std::shared_ptr<GameState>& initial_state, const SolverOptions& options);

	// Calls Solve() with the Floatiest Platforms level.
	std::shared_ptr<GameState> SolveFloatiestPlatforms(const SolverOptions& options);

}  // namespace BabaSolver
