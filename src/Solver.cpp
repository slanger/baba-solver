#include "Solver.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <limits>
#include <memory>
#include <mutex>
#include <print>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "GameState.h"
#include "NumberFormat.h"

namespace BabaSolver {

namespace {

// A struct to describe a future move, with an initial state and a direction to
// apply on top of that state.
struct NextMove {
  std::shared_ptr<GameState> state;
  Direction dir_to_apply;
};

// Prints the given SolverOptions to stdout.
void PrintSolverOptions(const SolverOptions& options, bool print_iterations) {
  if (print_iterations) {
    std::println("  Iteration count: {}", options.iteration_count);
  }
  std::println("  Max move depth: {}",
               static_cast<uint32_t>(options.max_turn_depth));
  std::println("  Parallelism depth: {}",
               static_cast<uint32_t>(options.parallelism_depth));
  std::println("  Max cache depth: {}",
               static_cast<uint32_t>(options.max_cache_depth));
}

// Tries to solve the level in one iteration given the initial state and
// options. Returns the winning state if it's possible to win in one iteration.
// Otherwise, returns the state with the highest score at the end of the
// iteration.
IterationResult SolveOneIteration(
    const std::shared_ptr<GameState>& initial_state,
    const SolverOptions& options) {
  IterationResult result;
  result.initial_state = initial_state;
  std::println("Solving with initial state:");
  initial_state->PrintGrid();

  // Use DFS (stack) instead of BFS (queue) in order to keep memory usage low.
  // The move tree is much, much wider than it is high, so using a depth-first
  // search will keep the stack size small compared to the enormous size that a
  // breadth-first search queue would need.
  std::stack<NextMove> stack;
  // Add the initial four directions to the stack.
  stack.push(NextMove{initial_state, Direction::UP});
  stack.push(NextMove{initial_state, Direction::RIGHT});
  stack.push(NextMove{initial_state, Direction::DOWN});
  stack.push(NextMove{initial_state, Direction::LEFT});
  // This unordered_map acts a cache of previously computed game states. If we
  // see a game state that we've already computed before, we don't compute that
  // game state again, potentially pruning a large chunk of the move tree.
  std::unordered_map<std::shared_ptr<GameState>, uint8_t, GameStatePtrHasher,
                     GameStatePtrComparer>
      seen_states;
  seen_states[initial_state] = 0;

  std::shared_ptr<GameState> winning_state;
  // parallelism_roots stores the game states at which we will start the
  // parallel algorithm (one thread per GameState in parallelism_roots).
  std::vector<std::shared_ptr<GameState>> parallelism_roots;
  auto start_time = std::chrono::high_resolution_clock::now();

  while (!stack.empty()) {
    ++result.total_num_moves;
    if (result.total_num_moves % options.print_every_n_moves == 0) {
      std::println(
          "Calculating move #{} ({}), cache size = {} ({}), stack size = {}",
          result.total_num_moves,
          FormatNumberWithSuffix(result.total_num_moves), seen_states.size(),
          FormatNumberWithSuffix(seen_states.size()), stack.size());
    }

    // Compute the new game state.
    const NextMove& cur = stack.top();
    std::shared_ptr<GameState> new_state =
        cur.state->ApplyMove(cur.dir_to_apply);
    stack.pop();
    uint8_t turn_count = new_state->TurnCount();

    // Check if we've won.
    if (new_state->HaveWon()) {
      std::println("WIN!!! Turn #{}", static_cast<uint32_t>(turn_count));
      winning_state = new_state;
      break;
    }

    if (turn_count <= options.max_cache_depth) {
      // Check the cache and don't proceed if the new game state has already
      // been computed before.
      auto [iter, inserted] = seen_states.insert({new_state, turn_count});
      if (!inserted) {
        if (turn_count >= iter->second) {
          ++result.total_cache_hits;
          continue;
        }
        iter->second = turn_count;
      }
    }

    // If it's impossible to win from this GameState, then prune that part of
    // the tree.
    if (!new_state->CheckIfPossibleToWin()) {
      continue;
    }

    // If we've reached parallelism_depth, then store the game state in
    // parallelism_roots.
    if (turn_count >= options.parallelism_depth) {
      parallelism_roots.push_back(new_state);
      continue;
    }

    // Add the next moves to the stack.
    stack.push(NextMove{new_state, Direction::UP});
    stack.push(NextMove{new_state, Direction::RIGHT});
    stack.push(NextMove{new_state, Direction::DOWN});
    stack.push(NextMove{new_state, Direction::LEFT});
  }

  result.total_cache_size = seen_states.size();
  result.parallelism_roots_count = parallelism_roots.size();
  std::vector<std::shared_ptr<GameState>> best_leaf_states(
      parallelism_roots.size());
  if (!winning_state) {
    std::mutex mutex;
    uint16_t next_thread_id = 0;
    uint16_t num_threads_finished = 0;
    uint16_t total_num_threads =
        static_cast<uint16_t>(parallelism_roots.size());
    std::println(
        "Finished the sequential portion. Now parallelizing into {} threads.",
        total_num_threads);

    // Use std::for_each with std::execution::par to parallelize the algorithm.
    // You can think of it as one thread per element in parallelism_roots, but
    // in reality it's more complicated than that. See
    // https://en.cppreference.com/w/cpp/algorithm#Execution_policies for more
    // details.
    std::for_each(
        std::execution::par, parallelism_roots.begin(), parallelism_roots.end(),
        [&options, &result, &mutex, &seen_states, &winning_state,
         &best_leaf_states, &next_thread_id, &num_threads_finished,
         &total_num_threads](std::shared_ptr<GameState> state) {
          uint16_t thread_id = 0;
          {
            std::lock_guard<std::mutex> lock(mutex);
            thread_id = next_thread_id++;
          }

          // Use DFS (stack) instead of BFS (queue) in order to keep memory
          // usage low. The move tree is much, much wider than it is high, so
          // using a depth-first search will keep the stack size small compared
          // to the enormous size that a breadth-first search queue would need.
          std::stack<NextMove> stack;
          // Apply initial four directions to the stack.
          stack.push(NextMove{state, Direction::UP});
          stack.push(NextMove{state, Direction::RIGHT});
          stack.push(NextMove{state, Direction::DOWN});
          stack.push(NextMove{state, Direction::LEFT});

          // Copy seen_states to make a thread-local cache.
          std::unordered_map<std::shared_ptr<GameState>, uint8_t,
                             GameStatePtrHasher, GameStatePtrComparer>
              local_seen_states = seen_states;
          uint64_t num_moves = 0;
          uint64_t num_cache_hits = 0;
          uint64_t leaf_count = 0;
          int best_score = std::numeric_limits<int>::min();
          std::shared_ptr<GameState> best_leaf_state;

          while (!stack.empty()) {
            ++num_moves;
            if (num_moves % options.print_every_n_moves == 0) {
              // Lock the mutex so that print statements don't get jumbled.
              std::lock_guard<std::mutex> lock(mutex);
              std::println(
                  "Thread {}: Calculating move #{} ({}), cache size = {} ({}), "
                  "stack size = {}",
                  thread_id, num_moves, FormatNumberWithSuffix(num_moves),
                  local_seen_states.size(),
                  FormatNumberWithSuffix(local_seen_states.size()),
                  stack.size());
            }

            // Compute the new game state.
            const NextMove& cur = stack.top();
            std::shared_ptr<GameState> new_state =
                cur.state->ApplyMove(cur.dir_to_apply);
            stack.pop();
            uint8_t turn_count = new_state->TurnCount();

            // Check if we've won.
            if (new_state->HaveWon()) {
              std::lock_guard<std::mutex> lock(mutex);
              std::println("WIN!!! Turn #{}",
                           static_cast<uint32_t>(turn_count));
              winning_state = new_state;
              best_leaf_state = new_state;
              break;
            }

            if (turn_count <= options.max_cache_depth) {
              // Check the cache and don't proceed if the new game state has
              // already been computed before.
              auto [iter, inserted] =
                  local_seen_states.insert({new_state, turn_count});
              if (!inserted) {
                if (turn_count >= iter->second) {
                  ++num_cache_hits;
                  continue;
                }
                iter->second = turn_count;
              }
            }

            // If it's impossible to win from this GameState, then prune that
            // part of the tree.
            if (!new_state->CheckIfPossibleToWin()) {
              continue;
            }

            // If we've reached max_turn_depth, then this game state is a leaf
            // in the tree. Calculate the score of this game state and see if
            // it's the best leaf state we've seen.
            if (turn_count >= options.max_turn_depth) {
              ++leaf_count;
              int score = new_state->CalculateScore();
              if (score > best_score) {
                best_score = score;
                best_leaf_state = new_state;
              }
              continue;
            }

            // Add the next moves to the stack.
            stack.push(NextMove{new_state, Direction::UP});
            stack.push(NextMove{new_state, Direction::RIGHT});
            stack.push(NextMove{new_state, Direction::DOWN});
            stack.push(NextMove{new_state, Direction::LEFT});
          }

          // Thread finished - print results.
          {
            std::lock_guard<std::mutex> lock(mutex);
            uint16_t finished_thread_count = ++num_threads_finished;
            result.total_num_moves += num_moves;
            result.total_cache_hits += num_cache_hits;
            result.total_cache_size += local_seen_states.size();
            result.leaf_state_count += leaf_count;
            best_leaf_states[thread_id] = best_leaf_state;
            // Print inside the critical section so that print statements don't
            // get jumbled.
            std::println(
                "Thread {} finished ({}/{}): Moves={}, Cache={}, Leaves={}",
                thread_id, finished_thread_count, total_num_threads,
                FormatNumberWithSuffix(num_moves),
                FormatNumberWithSuffix(local_seen_states.size()),
                FormatNumberWithSuffix(leaf_count));
          }
        });
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  result.total_duration = end_time - start_time;

  // Print results
  std::println();
  std::println("~~~ RESULTS ~~~");
  if (winning_state) {
    std::println("WIN!!! Winning state:");
    winning_state->PrintGrid();
    winning_state->PrintMoves();
    result.end_state = winning_state;
  } else {
    std::println("Did not win...");
    int best_score = std::numeric_limits<int>::min();
    std::shared_ptr<GameState> best_leaf_state;
    for (const std::shared_ptr<GameState>& leaf_state : best_leaf_states) {
      if (!leaf_state) continue;
      int score = leaf_state->CalculateScore();
      if (score > best_score) {
        best_score = score;
        best_leaf_state = leaf_state;
      }
    }
    if (best_leaf_state) {
      std::println("Best leaf game state:");
      best_leaf_state->PrintGrid();
      best_leaf_state->PrintMoves();
    } else {
      std::println("No leaf game states available.");
    }
    result.end_state = best_leaf_state;
  }
  std::println("Config:");
  PrintSolverOptions(options, false);
  std::println("Stats:");
  result.Print();
  std::println();

  return result;
}

}  // namespace

SolverResult Solve(std::string_view level_name,
                   const std::shared_ptr<GameState>& initial_state,
                   const SolverOptions& options) {
  SolverResult result;
  result.level_name = level_name;
  result.options = options;
  if (options.max_turn_depth > MAX_TURN_COUNT) {
    std::println("max_turn_depth must be less than MAX_TURN_COUNT ({})",
                 MAX_TURN_COUNT);
    return result;
  }

  std::println("Solving level \"{}\" with the following config options:",
               level_name);
  PrintSolverOptions(options, true);

  std::shared_ptr<GameState> current_state = initial_state;
  result.iterations.reserve(options.iteration_count);
  for (int i = 0; i < options.iteration_count; ++i) {
    std::println();
    std::println("======== ITERATION {} ========", i + 1);
    std::println();
	std::shared_ptr<GameState> new_state = current_state->Clone();
    new_state->ResetContext();
    result.iterations.push_back(SolveOneIteration(new_state, options));
    const IterationResult& iter_result = result.iterations.back();
    if (!iter_result.end_state) {
      std::println("No more options left. Exiting.");
      return result;
    }

    if (iter_result.end_state->HaveWon()) {
      result.solved = true;
      break;
    }

    current_state = iter_result.end_state;
  }
  return result;
}

void IterationResult::Print() const {
  std::println("  Total number of moves simulated (including cache hits): {}",
               FormatNumberWithCommas(this->total_num_moves));
  std::println("  Cache size: {} moves",
               FormatNumberWithCommas(this->total_cache_size));
  std::println("  Number of cache hits: {}",
               FormatNumberWithCommas(this->total_cache_hits));
  std::println(
      "  Number of unique, non-cached moves: {}",
      FormatNumberWithCommas(this->total_num_moves - this->total_cache_hits));
  std::println("  Number of parallel tree roots: {}",
               FormatNumberWithCommas(this->parallelism_roots_count));
  std::println("  Number of tree leaf game states: {}",
               FormatNumberWithCommas(this->leaf_state_count));
  std::println(
      "  Total time: {} seconds",
      std::chrono::duration_cast<std::chrono::seconds>(this->total_duration)
          .count());
  std::println("  Time per move: {} nanoseconds",
               this->total_duration.count() / this->total_num_moves);
  std::println("  End state:");
  this->end_state->PrintGrid();
}

void SolverOptions::Override(const SolverOptions& overrides) {
  if (overrides.iteration_count != 0)
    iteration_count = overrides.iteration_count;
  if (overrides.max_turn_depth != 0) max_turn_depth = overrides.max_turn_depth;
  if (overrides.parallelism_depth != 0)
    parallelism_depth = overrides.parallelism_depth;
  if (overrides.max_cache_depth != 0)
    max_cache_depth = overrides.max_cache_depth;
  if (overrides.print_every_n_moves != 0)
    print_every_n_moves = overrides.print_every_n_moves;
}

void SolverOptions::Print() const { PrintSolverOptions(*this, true); }

}  // namespace BabaSolver
