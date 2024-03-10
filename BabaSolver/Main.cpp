// BabaSolver solves the level "The Floatiest Platforms" in the game Baba Is You.
//
// The algorithm is a brute force algorithm, calculating all the moves you can do and printing out
// the first one that succeeds in beating the level. Various optimizations and heuristics prune the
// tree of possible game states for better performance.
//
// For best performance:
// * Run the program in "release" mode, not "debug" mode.
// * Tune the parameters in Parameters.h.

// TODO:
// * Implement "rock is push" rule, including breaking the rule. DONE
// * Allow Baba to die if it falls off the platform. DONE
// * Implement win condition. (Shortcut: Key touching door wins.) DONE
// * Add heuristic for preventing back-and-forth moves. OBSOLETE (see next item)
// * Prune sub-trees based on if we've already calculated them. DONE
// * Get rid of outer perimeter of immovable objects. DONE
// * Parallelize the algorithm. DONE

#include <cstdlib>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "GameState.h"
#include "Solver.h"

static void PrintHelp()
{
	std::string help = R"(BabaSolver: A program for solving Baba Is You levels

Usage: BabaSolver [--flag=<value> ...]

Flags:
  --parallelism_depth    The depth in the move tree at which the algorithm switches from single-threaded to multi-threaded. A higher value means a faster time to complete at the expense of more CPU and memory usage.
  --max_cache_depth      The max depth in the move tree at which to cache game states. A higher value trades CPU usage for memory usage.
  --print_every_n_moves  How often (in number of moves) to print a debug log to stdout.
  --help                 Prints this help message.
)";
	std::cout << help << std::endl;
}

int main(int argc, char* argv[])
{
	std::cout << "Baba Is You solver" << std::endl;
	BabaSolver::SolverOptions options;
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
	std::shared_ptr<BabaSolver::GameState> initial_state = BabaSolver::FloatiestPlatformsLevel();
	BabaSolver::Solve(initial_state, options);
	return 0;
}
