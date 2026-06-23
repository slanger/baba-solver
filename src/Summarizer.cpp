#include "Summarizer.h"

#include <print>

#include "Solver.h"

namespace BabaSolver {

void Summarize(const SolverResult& result) {
  // TODO: Sum up all iteration times and simulated move counts and print them.
  std::println(R"(
~~~ Summary ~~~

Level: {}
Level solved? {}

Solver options:)",
               result.level_name, result.solved ? "YES" : "NO");
  result.options.Print();
  std::println("\nInitial state:");
  result.iterations[0].initial_state->PrintGrid();
  std::println("\nPer iteration stats:");
  int iter_count = 0;
  for (const IterationResult& iter : result.iterations) {
    std::println("\nIteration {}", ++iter_count);
    iter.Print();
    std::println("End state:");
    iter.end_state->PrintGrid();
  }
}

}  // namespace BabaSolver
