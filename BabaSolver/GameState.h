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

namespace BabaSolver
{
	// The max number of turns this GameState can support.
	inline constexpr int MAX_TURN_COUNT = 30;

	// Level-specific constants
	inline constexpr int8_t GRID_HEIGHT = 18;
	inline constexpr int8_t GRID_WIDTH = 18;
	inline constexpr int16_t GRID_CELL_COUNT = GRID_HEIGHT * GRID_WIDTH;

	// Coordinate represents a point in a GameState's grid.
	struct Coordinate
	{
		int8_t i;
		int8_t j;
	};

	// Represents the direction a character is facing.
	enum class Direction : uint8_t
	{
		NO_DIRECTION,  // Special, used to indicate no direction
		UP,
		RIGHT,
		DOWN,
		LEFT,
	};

	// Represents different types of game objects (rock, door, Baba, etc).
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

	// GameState is the object for storing and manipulating game states. A GameState contains a 2D
	// grid of GameObjects representing the state of the game. A GameState also contains
	// "contextual" member variables, e.g. the turn count and which moves have been done to get to
	// the current GameState.
	class GameState
	{
	public:
		// State variables
		// A 2D grid containing the states of all GameObjects. Each cell in the grid is a uint16_t
		// holding a bitmask of which GameObjects are in the cell and which aren't. See
		// GameState.cpp for the more details about how this bitmask is implemented.
		uint16_t _grid[GRID_HEIGHT][GRID_WIDTH];
		// The location in the grid of Baba #1.
		Coordinate _baba1;
		// The location in the grid of Baba #2.
		Coordinate _baba2;

		// "Context" variables
		// How many turns have there been between this GameState and the initial GameState.
		uint8_t _turn;
		// A history of moves of how to get from the initial GameState to this GameState.
		Direction _moves[MAX_TURN_COUNT];

	private:
		// Cached state variables
		// The location of the key.
		Coordinate _key;
		// The location of the "IS" text block.
		Coordinate _is_text;
		// True if the "ROCK IS PUSH" rule is active, false otherwise.
		bool _rock_is_push_active;

	public:
		// Constructor. Takes in the initial state of the grid and Babas.
		GameState(uint16_t grid[GRID_HEIGHT][GRID_WIDTH], Coordinate baba1, Coordinate baba2);

		// Copy constructor. Makes a deep copy of all class member variables.
		GameState(const GameState& other);

		// Resets the "context" member variables (e.g. turn count and move history).
		void ResetContext();

		// Applies the given move to the current state and returns the resulting GameState. Does
		// *not* modify this GameState.
		std::shared_ptr<GameState> ApplyMove(Direction direction) const;

		// Returns true if this GameState is a winning state, false otherwise.
		bool HaveWon() const;

		// Returns true if it's possible to reach a winning state from this GameState, false
		// otherwise.
		bool CheckIfPossibleToWin() const;

		// Calculates the "score" of this GameState, which represents how likely the GameState will
		// lead a winning game state.
		int CalculateScore() const;

		// Prints the state of the grid to stdout.
		void PrintGrid() const;

		// Prints the history of moves to stdout.
		void PrintMoves() const;

	private:
		// Moves the given Baba in the given direction, applying all the relevant game rules (e.g.
		// pushing text blocks).
		void MoveBaba(Coordinate& baba, Direction direction);

		// Checks the cell in the direction of (delta_i, delta_j) to see if it's open, then, if the
		// cell is open, moves all the movable GameObjects from the cell (i, j) to the new cell. If
		// the new cell has movable GameObjects, this method is called recursively on that cell.
		bool CheckCellAndMoveObjects(int8_t i, int8_t j, int8_t delta_i, int8_t delta_j, uint16_t prev_cell);

		// Recalculates various internal state (e.g. whether or not a rule is still intact) after a
		// move has been made.
		void RecalculateState();

		// Checks if cell (i+delta_i, j+delta_j) is outside the bounds of the 2D grid.
		bool WillGoOutOfBounds(int8_t i, int8_t j, int8_t delta_i, int8_t delta_j) const;

		// Checks if all Babas are alive.
		bool AllBabasAlive() const;

		// Checks if the Babas are in the same cell of the grid.
		bool BabasOnSameSpace() const;

		// Checks if the "ROCK IS PUSH" text blocks are intact (and thus the rule is active).
		bool CheckRockIsPushIntact() const;

		// Checks if it is possible for the text blocks to be moved into the same row as the rocks.
		// This is used as an optimization for pruning paths in the move tree that won't lead to a
		// winning game state.
		bool CheckIfTextCanBeAlignedWithRocks(int8_t rock_row) const;
	};

	// Function object for hashing GameStates for a hash map.
	struct GameStateHash
	{
		std::size_t operator()(const std::shared_ptr<GameState>& state) const;
	};

	// Function object for comparing GameStates for equality.
	struct GameStateEqual
	{
		bool operator()(const std::shared_ptr<GameState>& lhs, const std::shared_ptr<GameState>& rhs) const;
	};

	// Creates the Floatiest Platforms level.
	std::shared_ptr<GameState> FloatiestPlatformsLevel();

	// Creates a level for testing.
	std::shared_ptr<GameState> TestLevel();

}  // namespace BabaSolver
