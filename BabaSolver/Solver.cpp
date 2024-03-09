#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <execution>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "GameState.h"
#include "Parameters.h"

#include "Solver.h"

namespace BabaSolver
{
	struct NextMove
	{
		std::shared_ptr<GameState> state;
		Direction dir_to_apply;
	};

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

	std::shared_ptr<GameState> Solve(const std::shared_ptr<GameState>& initial_state)
	{
		std::cout << "Solving with initial state:\n";
		initial_state->Print();

		std::stack<NextMove> stack;
		// Apply initial four directions to the stack.
		stack.push(NextMove{ initial_state, Direction::UP });
		stack.push(NextMove{ initial_state, Direction::RIGHT });
		stack.push(NextMove{ initial_state, Direction::DOWN });
		stack.push(NextMove{ initial_state, Direction::LEFT });
		std::unordered_set<std::shared_ptr<GameState>, GameStateHash, GameStateEqual> seen_states;
		seen_states.insert(initial_state);

		uint64_t total_num_moves = 0;
		uint64_t total_cache_hits = 0;
		std::shared_ptr<GameState> winning_state;
		std::vector<std::shared_ptr<GameState>> parallelism_roots;
		auto start_time = std::chrono::high_resolution_clock::now();
		while (!stack.empty())
		{
			++total_num_moves;
			if (total_num_moves % PRINT_EVERY_N_MOVES == 0)
			{
				std::cout << "Calculating move #" << total_num_moves << " (" << FormatNumberWithSuffix(total_num_moves)
					<< "), cache size = " << seen_states.size() << " (" << FormatNumberWithSuffix(seen_states.size())
					<< "), stack size = " << stack.size() << std::endl;
			}

			const NextMove& cur = stack.top();
			std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
			stack.pop();
			if (new_state->HaveWon())
			{
				std::cout << "WIN!!! Turn #" << new_state->_turn << "\n";
				winning_state = new_state;
				break;
			}
			// Heuristic: If the Babas are on the same space, then we're close to winning and can end
			// the search here.
			if (new_state->BabasOnSameSpace())
			{
				std::cout << "Found invincible Babas trick!!! Turn #" << new_state->_turn << "\n";
				winning_state = new_state;
				break;
			}

			if (new_state->_turn <= CACHED_MOVES_MAX_TURN_DEPTH)
			{
				if (seen_states.contains(new_state))
				{
					++total_cache_hits;
					continue;
				}
				seen_states.insert(new_state);
			}

			// Heuristic: If one of the Babas dies, then prune that part of the tree.
			if (!new_state->AllBabasAlive())
			{
				continue;
			}

			if (new_state->_turn >= PARALLELISM_DEPTH)
			{
				parallelism_roots.push_back(new_state);
				continue;
			}
			stack.push(NextMove{ new_state, Direction::UP });
			stack.push(NextMove{ new_state, Direction::RIGHT });
			stack.push(NextMove{ new_state, Direction::DOWN });
			stack.push(NextMove{ new_state, Direction::LEFT });
			new_state.reset();
		}

		std::size_t total_cache_size = seen_states.size();
		uint64_t leaf_state_count = 0;
		if (!winning_state)
		{
			std::mutex mutex;
			uint16_t next_thread_id = 0;
			uint16_t num_threads_finished = 0;
			uint16_t total_num_threads = static_cast<uint16_t>(parallelism_roots.size());
			std::cout << "Finished the sequential portion. Now parallelizing into " << total_num_threads << " threads." << std::endl;
			std::for_each(std::execution::par, parallelism_roots.begin(), parallelism_roots.end(),
				[&mutex, &seen_states, &winning_state, &total_num_moves, &total_cache_hits, &total_cache_size, &leaf_state_count, &next_thread_id, &num_threads_finished, &total_num_threads](std::shared_ptr<GameState> state)
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

					std::unordered_set<std::shared_ptr<GameState>, GameStateHash, GameStateEqual> local_seen_states = seen_states;
					uint64_t num_moves = 0;
					uint64_t num_cache_hits = 0;
					uint64_t leaf_count = 0;
					while (!stack.empty())
					{
						++num_moves;
						if (num_moves % PRINT_EVERY_N_MOVES == 0)
						{
							// Lock the mutex so that print statements don't get jumbled.
							std::lock_guard<std::mutex> lock(mutex);
							std::cout << "Thread " << thread_id << ": Calculating move #" << num_moves << " (" << FormatNumberWithSuffix(num_moves)
								<< "), cache size = " << local_seen_states.size() << " (" << FormatNumberWithSuffix(local_seen_states.size())
								<< "), stack size = " << stack.size() << std::endl;
						}

						const NextMove& cur = stack.top();
						std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
						stack.pop();
						if (new_state->HaveWon())
						{
							std::lock_guard<std::mutex> lock(mutex);
							std::cout << "WIN!!! Turn #" << new_state->_turn << "\n";
							winning_state = new_state;
							break;
						}
						// Heuristic: If the Babas are on the same space, then we're close to winning and can end
						// the search here.
						if (new_state->BabasOnSameSpace())
						{
							std::lock_guard<std::mutex> lock(mutex);
							std::cout << "Found invincible Babas trick!!! Turn #" << new_state->_turn << "\n";
							winning_state = new_state;
							break;
						}

						if (new_state->_turn <= CACHED_MOVES_MAX_TURN_DEPTH)
						{
							if (local_seen_states.contains(new_state))
							{
								++num_cache_hits;
								continue;
							}
							local_seen_states.insert(new_state);
						}

						// Heuristic: If one of the Babas dies, then prune that part of the tree.
						if (!new_state->AllBabasAlive())
						{
							continue;
						}

						if (new_state->_turn >= MAX_TURN_DEPTH)
						{
							++leaf_count;
							continue;
						}
						stack.push(NextMove{ new_state, Direction::UP });
						stack.push(NextMove{ new_state, Direction::RIGHT });
						stack.push(NextMove{ new_state, Direction::DOWN });
						stack.push(NextMove{ new_state, Direction::LEFT });
						new_state.reset();
					}

					// Thread finished - print results.
					{
						std::lock_guard<std::mutex> lock(mutex);
						uint16_t finished_thread_count = ++num_threads_finished;
						total_num_moves += num_moves;
						total_cache_hits += num_cache_hits;
						total_cache_size += local_seen_states.size();
						leaf_state_count += leaf_count;
						// Print inside the critical section so that print statements don't get jumbled.
						std::cout << "Thread " << thread_id << " finished (" << finished_thread_count << "/" << total_num_threads << "): Moves="
							<< FormatNumberWithSuffix(num_moves) << ", Cache=" << FormatNumberWithSuffix(local_seen_states.size()) << ", Leaves="
							<< FormatNumberWithSuffix(leaf_count) << std::endl;
					}
				});
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto total_time = end_time - start_time;

		// Print results
		std::cout << "\n=== RESULTS ===\n";
		if (winning_state)
		{
			std::cout << "WIN!!! Winning state:\n";
			winning_state->Print();
		}
		else
		{
			std::cout << "Did not win...\n";
		}
		std::cout << "Config:\n";
		std::cout << "  Max move depth: " << MAX_TURN_DEPTH << "\n";
		std::cout << "  Parallelism depth: " << PARALLELISM_DEPTH << "\n";
		std::cout << "  Max cache depth: " << CACHED_MOVES_MAX_TURN_DEPTH << "\n";
		std::cout << "Stats:\n";
		std::cout << "  Total number of moves simulated (including cache hits): " << FormatNumberWithCommas(total_num_moves) << "\n";
		std::cout << "  Cache size: " << FormatNumberWithCommas(total_cache_size) << " moves\n";
		std::cout << "  Number of cache hits: " << FormatNumberWithCommas(total_cache_hits) << "\n";
		std::cout << "  Number of unique, non-cached moves: " << FormatNumberWithCommas(total_num_moves - total_cache_hits) << "\n";
		std::cout << "  Number of parallel tree roots: " << FormatNumberWithCommas(parallelism_roots.size()) << "\n";
		std::cout << "  Number of tree leaf game states: " << FormatNumberWithCommas(leaf_state_count) << "\n";
		std::cout << "  Total time: " << std::chrono::duration_cast<std::chrono::seconds>(total_time).count() << " seconds\n";
		std::cout << "  Time per move: " << (total_time.count() / total_num_moves) << " nanoseconds\n";
		std::cout << std::endl;

		return winning_state;
	}

}  // namespace BabaSolver
