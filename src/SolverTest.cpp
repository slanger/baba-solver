// Tests for the Solver class.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>

#include "gtest/gtest.h"

#include "GameState.h"
#include "Solver.h"

namespace
{
	inline constexpr int8_t GRID_HEIGHT = 3;
	inline constexpr int8_t GRID_WIDTH = 3;

	uint16_t CombineUInt8s(uint8_t n1, uint8_t n2)
	{
		return (static_cast<uint16_t>(n1) << 8) | n2;
	}

	// GameStateTest is a test implementation of the GameState interface.
	class GameStateTest : public BabaSolver::GameState
	{
	public:
		BabaSolver::Coordinate _baba;
		uint8_t _turn;

		GameStateTest() : _baba({0, 0}), _turn(0) {}

		std::size_t Hash() const override
		{
			return std::hash<uint16_t>{}(CombineUInt8s(_baba.i, _baba.j));
		}

		bool Equals(const GameState& other) const override
		{
			if (const GameStateTest* rhs = dynamic_cast<const GameStateTest*>(&other))
			{
				return _baba.i == rhs->_baba.i && _baba.j == rhs->_baba.j;
			}
			return false;
		}

		uint8_t TurnCount() const override
		{
			return _turn;
		}

		std::shared_ptr<GameState> ApplyMove(BabaSolver::Direction direction) const override
		{
			std::shared_ptr<GameStateTest> new_state = std::make_shared<GameStateTest>(*this);
			int8_t i = _baba.i;
			int8_t j = _baba.j;
			switch (direction)
			{
			case BabaSolver::Direction::UP:
				new_state->_baba.i = (i == 0) ? 0 : i - 1;
				break;
			case BabaSolver::Direction::DOWN:
				new_state->_baba.i = (i == GRID_HEIGHT-1) ? GRID_HEIGHT-1 : i + 1;
				break;
			case BabaSolver::Direction::LEFT:
				new_state->_baba.j = (j == 0) ? 0 : j - 1;
				break;
			case BabaSolver::Direction::RIGHT:
				new_state->_baba.j = (j == GRID_WIDTH-1) ? GRID_WIDTH-1 : j + 1;
				break;
			default:
				std::cerr << "Invalid Direction: " << static_cast<uint8_t>(direction) << std::endl;
				break;
			}
			new_state->_turn += 1;
			return new_state;
		}

		bool HaveWon() const override
		{
			return _baba.i == GRID_HEIGHT-1 && _baba.j == GRID_WIDTH-1;
		}

		bool CheckIfPossibleToWin() const override
		{
			// Make the middle space "impossible" to win from.
			return !(_baba.i == 1 && _baba.j == 1);
		}

		int CalculateScore() const override
		{
			constexpr int max_distance = (GRID_HEIGHT - 1) + (GRID_WIDTH - 1);
			int distance = std::abs(_baba.i - (GRID_HEIGHT - 1)) + std::abs(_baba.j - (GRID_WIDTH - 1));
			return max_distance - distance;
		}

		void ResetContext() override
		{
			_turn = 0;
		}

		// Not implemented.
		void PrintGrid() const override {}
		void PrintMoves() const override {}
	};

}  // namespace


TEST(SolverTest, FindsSolution)
{
	std::shared_ptr<BabaSolver::GameState> initial_state = std::make_shared<GameStateTest>();
	BabaSolver::SolverOptions options{
		.iteration_count = 1,
		.max_turn_depth = 10,
		.parallelism_depth = 2,
		.max_cache_depth = 10,
		.print_every_n_moves = 10,
	};
	std::shared_ptr<BabaSolver::GameState> end_state = BabaSolver::Solve("Test Level", initial_state, options);
	ASSERT_TRUE(end_state);
	EXPECT_TRUE(end_state->HaveWon());
}
