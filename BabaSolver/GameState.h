// Code for storing and updating game states.

#pragma once

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

#include "Parameters.h"

namespace BabaSolver
{
	// Level-specific constants
	constexpr uint8_t GRID_HEIGHT = 18;
	constexpr uint8_t GRID_WIDTH = 18;
	constexpr uint16_t GRID_CELL_COUNT = GRID_HEIGHT * GRID_WIDTH;

	struct Coordinate
	{
		uint8_t i;
		uint8_t j;
	};

	enum class Direction : uint8_t
	{
		NO_DIRECTION,  // Special, used to indicate no direction
		UP,
		RIGHT,
		DOWN,
		LEFT,
	};

	enum class GameObject : uint16_t
	{
		BABA,
		IMMOVABLE,
		TILE,
		ROCK,
		DOOR,
		KEY,
		ROCK_TEXT,
		IS_TEXT,
		PUSH_TEXT,
	};

	class GameState
	{
	public:
		uint8_t _turn;
		uint16_t _grid[GRID_HEIGHT][GRID_WIDTH];
		Direction _moves[MAX_TURN_DEPTH];
		Coordinate _baba1;
		Coordinate _baba2;

	private:
		Coordinate _is_text;
		bool _rock_is_push_active;

	public:
		GameState(Coordinate baba1, Coordinate baba2, Coordinate is_text, bool rock_is_push_active);

		GameState(uint8_t turn, uint16_t grid[GRID_HEIGHT][GRID_WIDTH], Direction moves[MAX_TURN_DEPTH],
			Coordinate baba1, Coordinate baba2, Coordinate is_text, bool rock_is_push_active);

		std::shared_ptr<GameState> ApplyMove(Direction direction);

		bool HaveWon() const;

		bool AllBabasAlive() const;

		bool BabasOnSameSpace() const;

		void Print() const;

	private:
		void MoveBaba(Coordinate& baba, Direction direction);

		bool CheckCellAndMoveObjects(uint8_t i, uint8_t j, int8_t delta_i, int8_t delta_j, uint16_t prev_cell);

		void RecalculateState();

		bool WillGoOutOfBounds(uint8_t i, uint8_t j, int8_t delta_i, int8_t delta_j) const;

		bool CheckRockIsPushIntact() const;
	};

	struct GameStateHash
	{
		std::size_t operator()(const std::shared_ptr<GameState>& state) const;
	};

	struct GameStateEqual
	{
		bool operator()(const std::shared_ptr<GameState>& lhs, const std::shared_ptr<GameState>& rhs) const;
	};

	std::shared_ptr<GameState> FloatiestPlatformsLevel();

	std::shared_ptr<GameState> TestLevel();

}  // namespace BabaSolver
