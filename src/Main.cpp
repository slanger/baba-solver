// BabaSolver solves levels from the video game "Baba Is You".
//
// The algorithm is a brute force algorithm, calculating all the moves you can make and printing out
// the first one that succeeds in beating the level. Various optimizations and heuristics prune the
// tree of possible game states for better performance.
//
// For best performance:
// * Build and run the program in "release" mode, not "debug" mode.
// * Tune the flag parameters based on your computer's hardware (CPU speed, number of cores/threads,
//   amount of memory). Some flags trade CPU for more memory usage and vice versa.

#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <string_view>

#include "GameStateMtn6.h"
#include "GameStateMtnE1.h"
#include "Solver.h"

static constexpr std::string_view LEVEL_MTN_6 = "MTN_6";
static constexpr std::string_view LEVEL_MTN_E1 = "MTN_E1";

static void PrintHelp()
{
	std::string help = R"(BabaSolver: A program for solving Baba Is You levels

See the README for a detailed description of the algorithm used.

Usage: BabaSolver [--flag=<value> ...]

Flags:
  --level                The level to solve. Valid options: MTN_6, MTN_E1
  --iteration_count      How many iterations to run the solver.
  --max_turn_depth       The max depth in the move tree the algorithm will go in one iteration. The number of moves calculated grows exponentially with this value.
  --parallelism_depth    The depth in the move tree at which the algorithm switches from single-threaded to multi-threaded. A higher value means higher parallelism (up to the limits of the computer's CPU), which generally leads to a faster time to complete at the expense of more CPU and memory usage.
  --max_cache_depth      The max depth in the move tree at which to cache game states. A higher value trades CPU usage for memory usage.
  --print_every_n_moves  How often (in number of moves) to print a debug log to stdout.
  --help                 Prints this help message.
)";
	std::cout << help << std::endl;
}

int main(int argc, char* argv[])
{
	std::cout << "Baba Is You solver" << std::endl;

	// Parse flags.
	BabaSolver::SolverOptions options;
	std::string level(LEVEL_MTN_6);
	std::regex level_regex("--level=([a-zA-Z0-9_]+)");
	std::regex iteration_count_regex("--iteration_count=(\\d+)");
	std::regex max_turn_depth_regex("--max_turn_depth=(\\d+)");
	std::regex parallelism_depth_regex("--parallelism_depth=(\\d+)");
	std::regex max_cache_depth_regex("--max_cache_depth=(\\d+)");
	std::regex print_every_n_moves_regex("--print_every_n_moves=(\\d+)");
	for (int i = 1; i < argc; ++i)
	{
		std::string flag_str(argv[i]);
		std::smatch matches;
		if (flag_str == "--help")
		{
			PrintHelp();
			return 1;
		}
		if (std::regex_match(flag_str, matches, level_regex))
		{
			level = matches[1];
			continue;
		}
		if (std::regex_match(flag_str, matches, iteration_count_regex))
		{
			options.iteration_count = std::stoi(matches[1]);
			continue;
		}
		if (std::regex_match(flag_str, matches, max_turn_depth_regex))
		{
			options.max_turn_depth = std::stoi(matches[1]);
			continue;
		}
		if (std::regex_match(flag_str, matches, parallelism_depth_regex))
		{
			options.parallelism_depth = std::stoi(matches[1]);
			continue;
		}
		if (std::regex_match(flag_str, matches, max_cache_depth_regex))
		{
			options.max_cache_depth = std::stoi(matches[1]);
			continue;
		}
		if (std::regex_match(flag_str, matches, print_every_n_moves_regex))
		{
			options.print_every_n_moves = std::stoi(matches[1]);
			continue;
		}
		std::cout << "Invalid argument: " << flag_str << std::endl;
		PrintHelp();
		return 1;
	}

	// Run solver.
	if (level == LEVEL_MTN_6)
	{
		// If an option wasn't overridden, then use the level specific default value.
		if (options.iteration_count == 0) options.iteration_count = 4;
		if (options.max_turn_depth == 0) options.max_turn_depth = 30;
		if (options.parallelism_depth == 0) options.parallelism_depth = 2;
		if (options.max_cache_depth == 0) options.max_cache_depth = 25;
		if (options.print_every_n_moves == 0) options.print_every_n_moves = 10'000'000;

		auto initial_state = std::make_shared<BabaSolver::GameStateMtn6>();
		BabaSolver::Solve("Mountaintop Level 6 - Floaty Platforms", initial_state, options);
	}
	else if (level == LEVEL_MTN_E1)
	{
		// If an option wasn't overridden, then use the level specific default value.
		if (options.iteration_count == 0) options.iteration_count = 4;
		if (options.max_turn_depth == 0) options.max_turn_depth = 25;
		if (options.parallelism_depth == 0) options.parallelism_depth = 2;
		if (options.max_cache_depth == 0) options.max_cache_depth = 20;
		if (options.print_every_n_moves == 0) options.print_every_n_moves = 10'000'000;

		auto initial_state = std::make_shared<BabaSolver::GameStateMtnE1>();
		BabaSolver::Solve("Mountaintop Level Extra-1 - The Floatiest Platforms", initial_state, options);
	}
	else
	{
		std::cout << "Invalid level: " << level << std::endl;
		PrintHelp();
		return 1;
	}
	return 0;
}
