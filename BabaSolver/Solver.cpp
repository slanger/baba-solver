#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "GameState.h"

#include "Solver.h"

namespace BabaSolver
{
	namespace
	{
		// A struct to describe a future move, with an initial state and a direction to apply on
		// top of that state.
		struct NextMove
		{
			std::shared_ptr<GameState> state;
			Direction dir_to_apply;
		};
	}  // namespace

	// Formats the given number with a suffix, e.g. 10,000,000 -> "10M".
	static std::string FormatNumberWithSuffix(uint64_t n)
	{
		if (n >= 1'000'000'000)
			return std::to_string(n / 1'000'000'000) + "B";
		if (n >= 1'000'000)
			return std::to_string(n / 1'000'000) + "M";
		if (n >= 1'000)
			return std::to_string(n / 1'000) + "K";
		return std::to_string(n);
	}

	// Formats the given number with commas, e.g. 10000000 -> "10,000,000".
	static std::string FormatNumberWithCommas(uint64_t n)
	{
		std::string s = std::to_string(n);
		int64_t size = s.length() - 3;
		while (size > 0)
		{
			s.insert(size, ",");
			size -= 3;
		}
		return s;
	}

	// Prints the given SolverOptions to stdout.
	static void PrintSolverOptions(const SolverOptions& options, bool print_iterations)
	{
		if (print_iterations)
		{
			std::cout << "  Iteration count: " << options.iteration_count << "\n";
		}
		std::cout << "  Max move depth: " << options.max_turn_depth << "\n";
		std::cout << "  Parallelism depth: " << options.parallelism_depth << "\n";
		std::cout << "  Max cache depth: " << options.max_cache_depth << "\n";
	}

	// Tries to solve the level in one iteration given the initial state and options. Returns
	// the winning state if it's possible to win in one iteration. Otherwise, returns the state
	// with the highest score at the end of the iteration.
	static std::shared_ptr<GameState> SolveOneIteration(
		const std::shared_ptr<GameState>& initial_state, const SolverOptions& options)
	{
		std::cout << "Solving with initial state:\n";
		initial_state->PrintGrid();

		std::stack<NextMove> stack;
		// Add the initial four directions to the stack.
		stack.push(NextMove{ initial_state, Direction::UP });
		stack.push(NextMove{ initial_state, Direction::RIGHT });
		stack.push(NextMove{ initial_state, Direction::DOWN });
		stack.push(NextMove{ initial_state, Direction::LEFT });
		// This unordered_map acts a cache of previously computed game states. If we see a game
		// state that we've already computed before, we don't compute that game state again,
		// potentially pruning a large chunk of the move tree.
		std::unordered_map<std::shared_ptr<GameState>, uint8_t, GameStateHash, GameStateEqual> seen_states;
		seen_states[initial_state] = 0;

		// TODO: Put all these stats variables in a struct.
		uint64_t total_num_moves = 0;
		uint64_t total_cache_hits = 0;
		std::shared_ptr<GameState> winning_state;
		// parallelism_roots stores the game states at which we will start the parallel
		// algorithm (one thread per GameState in parallelism_roots).
		std::vector<std::shared_ptr<GameState>> parallelism_roots;
		auto start_time = std::chrono::high_resolution_clock::now();

		while (!stack.empty())
		{
			++total_num_moves;
			if (total_num_moves % options.print_every_n_moves == 0)
			{
				std::cout << "Calculating move #" << total_num_moves << " (" << FormatNumberWithSuffix(total_num_moves)
					<< "), cache size = " << seen_states.size() << " (" << FormatNumberWithSuffix(seen_states.size())
					<< "), stack size = " << stack.size() << std::endl;
			}

			// Compute the new game state.
			const NextMove& cur = stack.top();
			std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
			stack.pop();

			// Check if we've won.
			if (new_state->HaveWon())
			{
				std::cout << "WIN!!! Turn #" << static_cast<uint32_t>(new_state->_turn) << "\n";
				winning_state = new_state;
				break;
			}

			if (new_state->_turn <= options.max_cache_depth)
			{
				// Check the cache and don't proceed if the new game state has already been
				// computed before.
				auto [iter, inserted] = seen_states.insert({new_state, new_state->_turn});
				if (!inserted)
				{
					if (new_state->_turn >= iter->second)
					{
						++total_cache_hits;
						continue;
					}
					iter->second = new_state->_turn;
				}
			}

			// If it's impossible to win from this GameState, then prune that part of the tree.
			if (!new_state->CheckIfPossibleToWin())
			{
				continue;
			}

			// If we've reached parallelism_depth, then store the game state in parallelism_roots.
			if (new_state->_turn >= options.parallelism_depth)
			{
				parallelism_roots.push_back(new_state);
				continue;
			}

			// Add the next moves to the stack.
			stack.push(NextMove{ new_state, Direction::UP });
			stack.push(NextMove{ new_state, Direction::RIGHT });
			stack.push(NextMove{ new_state, Direction::DOWN });
			stack.push(NextMove{ new_state, Direction::LEFT });
		}

		std::size_t total_cache_size = seen_states.size();
		uint64_t leaf_state_count = 0;
		std::vector<std::shared_ptr<GameState>> best_leaf_states(parallelism_roots.size());
		if (!winning_state)
		{
			std::mutex mutex;
			uint16_t next_thread_id = 0;
			uint16_t num_threads_finished = 0;
			uint16_t total_num_threads = static_cast<uint16_t>(parallelism_roots.size());
			std::cout << "Finished the sequential portion. Now parallelizing into " << total_num_threads << " threads." << std::endl;

			// Use std::for_each with std::execution::par to parallelize the algorithm. You can
			// think of it as one thread per element in parallelism_roots, but in reality it's more
			// complicated than that. See
			// https://en.cppreference.com/w/cpp/algorithm#Execution_policies for more details.
			std::for_each(std::execution::par, parallelism_roots.begin(), parallelism_roots.end(),
				[&options, &mutex, &seen_states, &winning_state, &total_num_moves, &total_cache_hits, &total_cache_size, &leaf_state_count, &best_leaf_states, &next_thread_id, &num_threads_finished, &total_num_threads](std::shared_ptr<GameState> state)
				{
					uint16_t thread_id = 0;
					{
						std::lock_guard<std::mutex> lock(mutex);
						thread_id = next_thread_id++;
					}

					std::stack<NextMove> stack;
					// Apply initial four directions to the stack.
					stack.push(NextMove{ state, Direction::UP });
					stack.push(NextMove{ state, Direction::RIGHT });
					stack.push(NextMove{ state, Direction::DOWN });
					stack.push(NextMove{ state, Direction::LEFT });

					// Copy seen_states to make a thread-local cache.
					std::unordered_map<std::shared_ptr<GameState>, uint8_t, GameStateHash, GameStateEqual> local_seen_states = seen_states;
					uint64_t num_moves = 0;
					uint64_t num_cache_hits = 0;
					uint64_t leaf_count = 0;
					int best_score = std::numeric_limits<int>::min();
					std::shared_ptr<GameState> best_leaf_state;

					while (!stack.empty())
					{
						++num_moves;
						if (num_moves % options.print_every_n_moves == 0)
						{
							// Lock the mutex so that print statements don't get jumbled.
							std::lock_guard<std::mutex> lock(mutex);
							std::cout << "Thread " << thread_id << ": Calculating move #" << num_moves << " (" << FormatNumberWithSuffix(num_moves)
								<< "), cache size = " << local_seen_states.size() << " (" << FormatNumberWithSuffix(local_seen_states.size())
								<< "), stack size = " << stack.size() << std::endl;
						}

						// Compute the new game state.
						const NextMove& cur = stack.top();
						std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
						stack.pop();

						// Check if we've won.
						if (new_state->HaveWon())
						{
							std::lock_guard<std::mutex> lock(mutex);
							std::cout << "WIN!!! Turn #" << static_cast<uint32_t>(new_state->_turn) << "\n";
							winning_state = new_state;
							best_leaf_state = new_state;
							break;
						}

						if (new_state->_turn <= options.max_cache_depth)
						{
							// Check the cache and don't proceed if the new game state has already
							// been computed before.
							auto [iter, inserted] = local_seen_states.insert({ new_state, new_state->_turn });
							if (!inserted)
							{
								if (new_state->_turn >= iter->second)
								{
									++num_cache_hits;
									continue;
								}
								iter->second = new_state->_turn;
							}
						}

						// If it's impossible to win from this GameState, then prune that part of
						// the tree.
						if (!new_state->CheckIfPossibleToWin())
						{
							continue;
						}

						// If we've reached max_turn_depth, then this game state is a leaf in the
						// tree. Calculate the score of this game state and see if it's the best
						// leaf state we've seen.
						if (new_state->_turn >= options.max_turn_depth)
						{
							++leaf_count;
							int score = new_state->CalculateScore();
							if (score > best_score)
							{
								best_score = score;
								best_leaf_state = new_state;
							}
							continue;
						}

						// Add the next moves to the stack.
						stack.push(NextMove{ new_state, Direction::UP });
						stack.push(NextMove{ new_state, Direction::RIGHT });
						stack.push(NextMove{ new_state, Direction::DOWN });
						stack.push(NextMove{ new_state, Direction::LEFT });
					}

					// Thread finished - print results.
					{
						std::lock_guard<std::mutex> lock(mutex);
						uint16_t finished_thread_count = ++num_threads_finished;
						total_num_moves += num_moves;
						total_cache_hits += num_cache_hits;
						total_cache_size += local_seen_states.size();
						leaf_state_count += leaf_count;
						best_leaf_states[thread_id] = best_leaf_state;
						// Print inside the critical section so that print statements don't get jumbled.
						std::cout << "Thread " << thread_id << " finished (" << finished_thread_count << "/" << total_num_threads << "): Moves="
							<< FormatNumberWithSuffix(num_moves) << ", Cache=" << FormatNumberWithSuffix(local_seen_states.size()) << ", Leaves="
							<< FormatNumberWithSuffix(leaf_count) << std::endl;
					}
				});
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto total_duration = end_time - start_time;

		// Print results
		std::cout << "\n~~~ RESULTS ~~~\n";
		if (winning_state)
		{
			std::cout << "WIN!!! Winning state:\n";
			winning_state->PrintGrid();
			winning_state->PrintMoves();
		}
		else
		{
			std::cout << "Did not win...\n";
			int best_score = std::numeric_limits<int>::min();
			std::shared_ptr<GameState> best_leaf_state;
			for (const std::shared_ptr<GameState>& leaf_state : best_leaf_states)
			{
				if (!leaf_state)
					continue;
				int score = leaf_state->CalculateScore();
				if (score > best_score)
				{
					best_score = score;
					best_leaf_state = leaf_state;
				}
			}
			std::cout << "Best leaf game state:\n";
			best_leaf_state->PrintGrid();
			best_leaf_state->PrintMoves();
			winning_state = best_leaf_state;
		}
		std::cout << "Config:\n";
		PrintSolverOptions(options, false);
		std::cout << "Stats:\n";
		std::cout << "  Total number of moves simulated (including cache hits): " << FormatNumberWithCommas(total_num_moves) << "\n";
		std::cout << "  Cache size: " << FormatNumberWithCommas(total_cache_size) << " moves\n";
		std::cout << "  Number of cache hits: " << FormatNumberWithCommas(total_cache_hits) << "\n";
		std::cout << "  Number of unique, non-cached moves: " << FormatNumberWithCommas(total_num_moves - total_cache_hits) << "\n";
		std::cout << "  Number of parallel tree roots: " << FormatNumberWithCommas(parallelism_roots.size()) << "\n";
		std::cout << "  Number of tree leaf game states: " << FormatNumberWithCommas(leaf_state_count) << "\n";
		std::cout << "  Total time: " << std::chrono::duration_cast<std::chrono::seconds>(total_duration).count() << " seconds\n";
		std::cout << "  Time per move: " << (total_duration.count() / total_num_moves) << " nanoseconds\n";
		std::cout << std::endl;

		return winning_state;
	}

	std::shared_ptr<GameState> Solve(std::string_view level_name,
		const std::shared_ptr<GameState>& initial_state, const SolverOptions& options)
	{
		if (options.max_turn_depth > MAX_TURN_COUNT)
		{
			std::cout << "max_turn_depth must be less than MAX_TURN_COUNT (" << MAX_TURN_COUNT << ")" << std::endl;
			return nullptr;
		}

		std::cout << "Solving level \"" << level_name << "\" with the following config options:\n";
		PrintSolverOptions(options, true);

		std::shared_ptr<GameState> current_state = initial_state;
		for (int i = 0; i < options.iteration_count; ++i)
		{
			std::cout << "\n======== ITERATION " << (i + 1) << " ========\n" << std::endl;
			current_state->ResetContext();
			current_state = SolveOneIteration(current_state, options);
			if (current_state->HaveWon())
				break;
		}
		return current_state;
	}

	std::shared_ptr<GameState> SolveFloatiestPlatforms(const SolverOptions& options)
	{
		return Solve("Floatiest Platforms", FloatiestPlatformsLevel(), options);
	}

}  // namespace BabaSolver
