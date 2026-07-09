// BabaSolver solves levels from the video game "Baba Is You".
//
// The algorithm is a brute force algorithm, calculating all the moves you can
// make and printing out the first one that succeeds in beating the level.
// Various optimizations and heuristics prune the tree of possible game states
// for better performance.
//
// For best performance:
// * Build and run the program in "release" mode, not "debug" mode.
// * Tune the flag parameters based on your computer's hardware (CPU speed,
//   number of cores/threads, amount of memory). Some flags trade CPU for more
//   memory usage and vice versa.

#include <memory>
#include <print>
#include <regex>
#include <string>
#include <string_view>

#include "GameState.h"
#include "GameStateMtn6.h"
#include "GameStateMtnE1.h"
#include "Replayer.h"
#include "Solver.h"
#include "Summarizer.h"

static constexpr std::string_view LEVEL_MTN_6 = "MTN_6";
static constexpr std::string_view LEVEL_MTN_E1 = "MTN_E1";

static constexpr std::string_view LEVEL_MTN_6_NAME =
    "Mountaintop Level 6 - Floaty Platforms";
static constexpr std::string_view LEVEL_MTN_E1_NAME =
    "Mountaintop Level Extra-1 - The Floatiest Platforms";

static void PrintHelp() {
  std::println(R"(BabaSolver: A program for solving Baba Is You levels

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
)");
}

int main(int argc, char* argv[]) {
  std::println("Baba Is You solver");

  // Parse flags.
  BabaSolver::SolverOptions overrides;
  std::string level(LEVEL_MTN_6);
  std::regex level_regex("--level=([a-zA-Z0-9_]+)");
  std::regex iteration_count_regex("--iteration_count=(\\d+)");
  std::regex max_turn_depth_regex("--max_turn_depth=(\\d+)");
  std::regex parallelism_depth_regex("--parallelism_depth=(\\d+)");
  std::regex max_cache_depth_regex("--max_cache_depth=(\\d+)");
  std::regex print_every_n_moves_regex("--print_every_n_moves=(\\d+)");
  for (int i = 1; i < argc; ++i) {
    std::string flag_str(argv[i]);
    std::smatch matches;
    if (flag_str == "--help") {
      PrintHelp();
      return 1;
    }
    if (std::regex_match(flag_str, matches, level_regex)) {
      level = matches[1];
      continue;
    }
    if (std::regex_match(flag_str, matches, iteration_count_regex)) {
      overrides.iteration_count = std::stoi(matches[1]);
      continue;
    }
    if (std::regex_match(flag_str, matches, max_turn_depth_regex)) {
      overrides.max_turn_depth = std::stoi(matches[1]);
      continue;
    }
    if (std::regex_match(flag_str, matches, parallelism_depth_regex)) {
      overrides.parallelism_depth = std::stoi(matches[1]);
      continue;
    }
    if (std::regex_match(flag_str, matches, max_cache_depth_regex)) {
      overrides.max_cache_depth = std::stoi(matches[1]);
      continue;
    }
    if (std::regex_match(flag_str, matches, print_every_n_moves_regex)) {
      overrides.print_every_n_moves = std::stoi(matches[1]);
      continue;
    }
    std::println("Invalid argument: {}", flag_str);
    PrintHelp();
    return 1;
  }

  // Get default options for level.
  std::string_view level_name;
  BabaSolver::SolverOptions options;
  std::shared_ptr<BabaSolver::GameState> initial_state;
  if (level == LEVEL_MTN_6) {
    level_name = LEVEL_MTN_6_NAME;
    options = BabaSolver::SolverOptions{
        .iteration_count = 4,
        .max_turn_depth = 30,
        .parallelism_depth = 2,
        .max_cache_depth = 25,
        .print_every_n_moves = 10'000'000,
    };
    initial_state = std::make_shared<BabaSolver::GameStateMtn6>();
  } else if (level == LEVEL_MTN_E1) {
    level_name = LEVEL_MTN_E1_NAME;
    options = BabaSolver::SolverOptions{
        .iteration_count = 4,
        .max_turn_depth = 25,
        .parallelism_depth = 2,
        .max_cache_depth = 20,
        .print_every_n_moves = 10'000'000,
    };
    initial_state = std::make_shared<BabaSolver::GameStateMtnE1>();
  } else {
    std::println("Invalid level: {}", level);
    PrintHelp();
    return 1;
  }

  // Run solver.
  // Override the level's default options with the command-line flag values, if
  // specified.
  options.Override(overrides);
  BabaSolver::SolverResult result =
      BabaSolver::Solve(level_name, initial_state, options);
  BabaSolver::SummarizeResult(result);
  BabaSolver::ReplayResult(result);
  return result.solved ? 0 : 1;
}
