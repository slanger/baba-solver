// Code for implementing game states for Level 6 of Mountaintop ("Floaty
// Platforms").

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "GameState.h"

namespace BabaSolver {

// GameState is the object for storing and manipulating game states. A GameState
// contains a 2D grid of GameObjects representing the state of the game. A
// GameState also contains "contextual" member variables, e.g. the turn count
// and which moves have been done to get to the current GameState.
class GameStateMtn6 : public GameState {
 private:
  // Level-specific constants
  static constexpr int8_t GRID_HEIGHT = 18;
  static constexpr int8_t GRID_WIDTH = 24;

 public:
  // State variables
  // A 2D grid containing the states of all GameObjects. Each cell in the grid
  // is a uint16_t holding a bitmask of which GameObjects are in the cell and
  // which aren't. See GameStateMtn6.cpp for the more details about how this
  // bitmask is implemented.
  uint16_t _grid[GRID_HEIGHT][GRID_WIDTH];
  // The location in the grid of Baba #1.
  Coordinate _baba1;
  // The location in the grid of Baba #2.
  Coordinate _baba2;

  // "Context" variables
  // How many turns have there been between this GameState and the initial
  // GameState.
  uint8_t _turn;
  // A history of moves of how to get from the initial GameState to this
  // GameState.
  Direction _moves[MAX_TURN_COUNT];

 private:
  // Cached state variables
  // The location of the door.
  Coordinate _door;
  // The location of the key.
  Coordinate _key;
  // The location of the "IS" text block.
  Coordinate _is_text;
  // True if the "ROCK IS PUSH" rule is active, false otherwise.
  bool _rock_is_push_active;

 public:
  // Constructor
  GameStateMtn6();

  // GameState interface implementations
  // See GameState.h for comments are what these functions do.

  std::shared_ptr<GameState> Clone() const override;
  std::size_t Hash() const override;
  bool Equals(const GameState& other) const override;
  uint8_t TurnCount() const override;
  std::vector<Direction> Moves() const override;
  std::shared_ptr<GameState> ApplyMove(Direction direction) const override;
  bool HaveWon() const override;
  bool CheckIfPossibleToWin() const override;
  int CalculateScore() const override;
  void PrintGrid() const override;
  void PrintMoves() const override;
  void ResetContext() override;

 private:
  // Moves the given Baba in the given direction, applying all the relevant game
  // rules (e.g. pushing text blocks).
  void MoveBaba(Coordinate& baba, Direction direction);

  // Checks the cell in the direction of (delta_i, delta_j) to see if it's open,
  // then, if the cell is open, moves all the movable GameObjects from the cell
  // (i, j) to the new cell. If the new cell has movable GameObjects, this
  // method is called recursively on that cell.
  bool CheckCellAndMoveObjects(int8_t i, int8_t j, int8_t delta_i,
                               int8_t delta_j, uint16_t prev_cell);

  // Recalculates various internal state (e.g. whether or not a rule is still
  // intact) after a move has been made.
  void RecalculateState();

  // Checks if cell (i+delta_i, j+delta_j) is outside the bounds of the 2D grid.
  bool WillGoOutOfBounds(int8_t i, int8_t j, int8_t delta_i,
                         int8_t delta_j) const;

  // Checks if all Babas are alive.
  bool AllBabasAlive() const;

  // Checks if the Babas are in the same cell of the grid.
  bool BabasOnSameSpace() const;

  // Checks if the "ROCK IS PUSH" text blocks are intact (and thus the rule is
  // active).
  bool CheckRockIsPushIntact() const;
};

}  // namespace BabaSolver
