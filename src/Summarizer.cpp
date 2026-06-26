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
  std::print(" STATS           ");
  for (int i = 1; i <= result.iterations.size(); ++i) {
    std::print("| {:^14} ", std::format("Iteration {}", i));
  }
  std::println("| {:^14} ", "Total");
  std::println("-----------------{}",
               std::string((result.iterations.size() + 1) * 17, '-'));
  // Moves simulated
  std::print(" Moves simulated ");
  uint64_t total_moves = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.total_num_moves));
    total_moves += iter.total_num_moves;
  }
  std::println("| {:>14} ", FormatNumberWithCommas(total_moves));
  // Cache size
  std::print(" Cache size      ");
  uint64_t total_cache_size = 0;
  for (const IterationResult& iter : result.iterations) {
    std::print("| {:>14} ", FormatNumberWithCommas(iter.total_cache_size));
    total_cache_size += iter.total_cache_size;
  }
  std::println("| {:>14} ", FormatNumberWithCommas(total_cache_size));
  // Total time
  std::print(" Total time      ");
  std::chrono::seconds total_time(0);
  for (const IterationResult& iter : result.iterations) {
    std::chrono::seconds secs =
        std::chrono::duration_cast<std::chrono::seconds>(iter.total_duration);
    std::print("| {:>14} ", secs);
    total_time += secs;
  }
  std::println("| {:>14} ", total_time);
  // TODO: Add the rest of the stats here.
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
