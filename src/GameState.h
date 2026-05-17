// Code for storing and updating game states.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace BabaSolver
{
	// The max number of turns GameState's can support.
	inline constexpr int MAX_TURN_COUNT = 35;

	// Coordinate represents a point in a GameState's grid.
	struct Coordinate
	{
		int8_t i;
		int8_t j;
	};

	// Represents the direction a character is facing.
	enum class Direction : uint8_t
	{
		NO_DIRECTION = 0,  // Special, used to indicate no direction
		UP,
		RIGHT,
		DOWN,
		LEFT,
	};

	// GameState is a pure virtual interface for the object that stores and manipulates game states.
	class GameState
	{
	public:
		// Returns the hash of this GameState object, for use in hash maps and sets.
		virtual std::size_t Hash() const = 0;

		// Compares this GameState object with another, and returns true if they're equal.
		virtual bool Equals(const GameState& other) const = 0;

		// Returns the number of turns this game state has been through so far.
		virtual uint8_t TurnCount() const = 0;

		// Applies the given move to the current state and returns the resulting GameState. Does
		// *not* modify this GameState.
		virtual std::shared_ptr<GameState> ApplyMove(Direction direction) const = 0;

		// Returns true if this GameState is a winning state, false otherwise.
		virtual bool HaveWon() const = 0;

		// Returns true if it's possible to reach a winning state from this GameState, false
		// otherwise.
		virtual bool CheckIfPossibleToWin() const = 0;

		// Calculates the "score" of this GameState, which represents how likely the GameState will
		// lead a winning game state.
		virtual int CalculateScore() const = 0;

		// Prints the state of the grid to stdout.
		virtual void PrintGrid() const = 0;

		// Prints the history of moves to stdout.
		virtual void PrintMoves() const = 0;

		// Resets the "context" member variables (e.g. turn count and move history).
		virtual void ResetContext() = 0;

		// Need a virtual destructor for this interface to be cleaned up properly.
		virtual ~GameState() = default;
	};

	// Function object for hashing GameStates (inside std::shared_ptr's) for a hash map/set.
	struct GameStatePtrHasher
	{
		std::size_t operator()(const std::shared_ptr<GameState>& state) const;
	};

	// Function object for comparing GameStates (inside std::shared_ptr's) for equality.
	struct GameStatePtrComparer
	{
		bool operator()(const std::shared_ptr<GameState>& lhs, const std::shared_ptr<GameState>& rhs) const;
	};

}  // namespace BabaSolver
