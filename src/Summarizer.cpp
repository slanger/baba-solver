#include "Summarizer.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <print>
#include <string>

#include "NumberFormat.h"
#include "Solver.h"

namespace BabaSolver {

static void PrintStats(const SolverResult& result) {
  // Table header
  std::print(" STATS            ");
  for (int i = 1; i <= result.iterations.size(); ++i) {
    std::print("| {:^14} ", std::format("Iteration {}", i));
  }
  std::println("|| {:^14} ", "Total");
  std::println("-------------------{}",
               std::string((result.iterations.size() + 1) * 17, '-'));

  // Moves simulated
  std::print(" Moves simulated  ");
  uint64_t total_moves = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.total_num_moves));
    total_moves += iter.total_num_moves;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_moves));

  // Cache size
  std::print(" Cache size       ");
  uint64_t total_cache_size = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.total_cache_size));
    total_cache_size += iter.total_cache_size;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_cache_size));

  // Number of cache hits
  std::print(" # cache hits     ");
  uint64_t total_cache_hits = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.total_cache_hits));
    total_cache_hits += iter.total_cache_hits;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_cache_hits));

  // Number of unique, non-cached moves
  std::print(" # unique moves   ");
  uint64_t total_unique_moves = 0;
  for (const IterationResult& iter : result.iterations) {
    uint64_t unique_moves = iter.total_num_moves - iter.total_cache_hits;
    std::print("| {:>14} ", FormatNumberWithCommas(unique_moves));
    total_unique_moves += unique_moves;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_unique_moves));

  // Number of parallel tree roots
  std::print(" # parallel roots ");
  int total_parallelism_roots = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.parallelism_roots_count));
    total_parallelism_roots += iter.parallelism_roots_count;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_parallelism_roots));

  // Number of tree leaf game states
  std::print(" # leaf states    ");
  uint64_t total_leaf_states = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.leaf_state_count));
    total_leaf_states += iter.leaf_state_count;
  }
  std::println("|| {:>14} ", FormatNumberWithCommas(total_leaf_states));

  // Total time
  std::print(" Total time       ");
  std::chrono::seconds total_time(0);
  for (const IterationResult& iter : result.iterations) {
    std::chrono::seconds secs =
        std::chrono::duration_cast<std::chrono::seconds>(iter.total_duration);
    std::print("| {:>14} ", secs);
    total_time += secs;
  }
  std::println("|| {:>14} ", total_time);

  // Time per move
  std::print(" Time per move    ");
  std::chrono::nanoseconds total_nsecs(0);
  for (const IterationResult& iter : result.iterations) {
    std::chrono::nanoseconds time_per_move = iter.total_duration / iter.total_num_moves;
    std::print("| {:>14} ", time_per_move);
    total_nsecs += iter.total_duration;
  }
  std::println("|| {:>14} ", total_nsecs / total_moves);
}

void Summarize(const SolverResult& result) {
  std::println(R"(
~~~ Summary ~~~

Level: {}
Level solved? {}

Solver options)",
               result.level_name, result.solved ? "YES" : "NO");
  result.options.Print();
  std::println();
  PrintStats(result);
  std::println();
  std::println("Initial state:");
  result.iterations[0].initial_state->PrintGrid();
  std::println();
  int iter_count = 0;
  for (const IterationResult& iter : result.iterations) {
    std::println("Iteration {} end state:", ++iter_count);
    iter.end_state->PrintGrid();
    std::println();
    // TODO: Print the list of moves for each iteration.
  }
}

}  // namespace BabaSolver
