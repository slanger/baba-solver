#include "GameStateMtn6.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <execution>
#include <memory>
#include <mutex>
#include <print>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "GameState.h"

namespace BabaSolver {

namespace {

// Represents different types of game objects (rock, door, Baba, etc).
enum class GameObject : uint16_t {
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

constexpr int8_t BABA_DEAD = -1;

constexpr GameObject ALWAYS_MOVABLE_OBJECTS[] = {
    GameObject::DOOR, GameObject::KEY, GameObject::ROCK_TEXT,
    GameObject::IS_TEXT, GameObject::PUSH_TEXT};

constexpr uint16_t IMMOVABLE_OBJECT_BITMASK =
    (1 << static_cast<uint16_t>(GameObject::IMMOVABLE));

constexpr uint16_t MOVABLE_OBJECT_BITMASK =
    (1 << static_cast<uint16_t>(GameObject::DOOR)) |
    (1 << static_cast<uint16_t>(GameObject::KEY)) |
    (1 << static_cast<uint16_t>(GameObject::ROCK_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::IS_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::PUSH_TEXT));

constexpr uint16_t TEXT_BITMASK =
    (1 << static_cast<uint16_t>(GameObject::ROCK_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::IS_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::PUSH_TEXT));

constexpr uint16_t ROCK_AND_TEXT_BITMASK =
    (1 << static_cast<uint16_t>(GameObject::ROCK)) |
    (1 << static_cast<uint16_t>(GameObject::ROCK_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::IS_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::PUSH_TEXT));

constexpr uint16_t KEY_AND_TEXT_BITMASK =
    (1 << static_cast<uint16_t>(GameObject::KEY)) |
    (1 << static_cast<uint16_t>(GameObject::ROCK_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::IS_TEXT)) |
    (1 << static_cast<uint16_t>(GameObject::PUSH_TEXT));

char GameObjectToChar(GameObject obj) {
  switch (obj) {
    case GameObject::BABA:
      return 'B';
    case GameObject::IMMOVABLE:
      return 'X';
    case GameObject::TILE:
      return '^';
    case GameObject::ROCK:
      return 'R';
    case GameObject::DOOR:
      return 'D';
    case GameObject::KEY:
      return 'K';
    case GameObject::ROCK_TEXT:
      return '1';
    case GameObject::IS_TEXT:
      return '2';
    case GameObject::PUSH_TEXT:
      return '3';
  }
  // Should not be able to reach this point.
  std::println(stderr, "GameObject isn't set in GameObjectToChar(): {}",
               static_cast<uint16_t>(obj));
  std::abort();
}

// Note: This does NOT check for Babas in the cell.
bool CellIsEmpty(uint16_t cell) { return cell == 0; }

bool CellContainsGameObject(uint16_t cell, GameObject obj) {
  uint16_t bitmask = 1 << static_cast<uint16_t>(obj);
  return (cell & bitmask) != 0;
}

bool CellContainsGameObjects(uint16_t cell, uint16_t obj_bitmask) {
  return (cell & obj_bitmask) != 0;
}

bool CellContainsImmovableObject(uint16_t cell) {
  return (cell & IMMOVABLE_OBJECT_BITMASK) != 0;
}

bool CellContainsMovableObject(uint16_t cell, bool rock_is_push_active) {
  if ((cell & MOVABLE_OBJECT_BITMASK) != 0) return true;
  if (rock_is_push_active && CellContainsGameObject(cell, GameObject::ROCK))
    return true;
  return false;
}

void AddToCellInPlace(uint16_t& cell, GameObject obj) {
  uint16_t bitmask = 1 << static_cast<uint16_t>(obj);
  cell |= bitmask;
}

uint16_t AddToCell(uint16_t cell, GameObject obj) {
  AddToCellInPlace(cell, obj);
  return cell;
}

void RemoveFromCellInPlace(uint16_t& cell, GameObject obj) {
  uint16_t bitmask = ~(1 << static_cast<uint16_t>(obj));
  cell &= bitmask;
}

uint16_t RemoveFromCell(uint16_t cell, GameObject obj) {
  RemoveFromCellInPlace(cell, obj);
  return cell;
}

uint16_t CombineUInt8s(uint8_t n1, uint8_t n2) {
  return (static_cast<uint16_t>(n1) << 8) | n2;
}

uint32_t CombineUInt16s(uint16_t n1, uint16_t n2) {
  return (static_cast<uint32_t>(n1) << 16) | n2;
}

uint64_t CombineUInt32s(uint32_t n1, uint32_t n2) {
  return (static_cast<uint64_t>(n1) << 32) | n2;
}

std::size_t ApplyHash(uint32_t to_apply, std::size_t current_hash) {
  // Note: I forgot where I got this code from, but it was on Stack Overflow
  // somewhere. I have no idea how good or bad it is.
  // TODO: Check how effective this hashing code is.
  to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
  to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
  to_apply = (to_apply >> 16) ^ to_apply;
  current_hash ^= static_cast<std::size_t>(to_apply) + 0x9e3779b9 +
                  (current_hash << 6) + (current_hash >> 2);
  return current_hash;
}

}  // namespace

GameStateMtn6::GameStateMtn6()
    : _grid{},
      _baba1({5, 7}),
      _baba2({5, 15}),
      _turn(0),
      _moves{},
      _door({12, 7}),
      _key({11, 15}),
      _is_text({4, 15}),
      _rock_is_push_active(true) {
  // Add tiles to grid.
  for (int8_t i = 3; i <= 7; ++i) {
    for (int8_t j = 5; j <= 9; ++j) {
      AddToCellInPlace(_grid[i][j], GameObject::TILE);
    }
  }
  for (int8_t i = 3; i <= 7; ++i) {
    for (int8_t j = 13; j <= 17; ++j) {
      AddToCellInPlace(_grid[i][j], GameObject::TILE);
    }
  }
  for (int8_t i = 10; i <= 14; ++i) {
    for (int8_t j = 5; j <= 9; ++j) {
      AddToCellInPlace(_grid[i][j], GameObject::TILE);
    }
  }
  for (int8_t i = 9; i <= 13; ++i) {
    for (int8_t j = 13; j <= 17; ++j) {
      AddToCellInPlace(_grid[i][j], GameObject::TILE);
    }
  }
  // Add an immovable for each text block around the corners of the grid.
  AddToCellInPlace(_grid[0][0], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[0][1], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[0][2], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[0][10], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[0][11], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[0][12], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][0], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][1], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][2], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][3], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][4], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][0], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][1], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][2], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][3], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[15][21], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[15][22], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[15][23], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][21], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][22], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[16][23], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][21], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][22], GameObject::IMMOVABLE);
  AddToCellInPlace(_grid[17][23], GameObject::IMMOVABLE);
  // Add the other game objects to the grid.
  AddToCellInPlace(_grid[4][6], GameObject::ROCK);
  AddToCellInPlace(_grid[6][8], GameObject::ROCK);
  AddToCellInPlace(_grid[6][14], GameObject::ROCK);
  AddToCellInPlace(_grid[4][14], GameObject::ROCK_TEXT);
  AddToCellInPlace(_grid[_is_text.i][_is_text.j], GameObject::IS_TEXT);
  AddToCellInPlace(_grid[4][16], GameObject::PUSH_TEXT);
  AddToCellInPlace(_grid[_door.i][_door.j], GameObject::DOOR);
  AddToCellInPlace(_grid[_key.i][_key.j], GameObject::KEY);

  RecalculateState();
}

GameStateMtn6::GameStateMtn6(const GameStateMtn6& other)
    : _baba1(other._baba1),
      _baba2(other._baba2),
      _turn(other._turn),
      _door(other._door),
      _key(other._key),
      _is_text(other._is_text),
      _rock_is_push_active(other._rock_is_push_active) {
  std::copy(&other._grid[0][0], &other._grid[0][0] + GRID_CELL_COUNT,
            &_grid[0][0]);
  std::copy(&other._moves[0], &other._moves[0] + other._turn, &_moves[0]);
}

std::shared_ptr<GameState> GameStateMtn6::Clone() const {
  return std::make_shared<GameStateMtn6>(*this);
}

std::size_t GameStateMtn6::Hash() const {
  std::size_t hash = static_cast<std::size_t>(
                         CombineUInt16s(CombineUInt8s(_baba1.i, _baba1.j),
                                        CombineUInt8s(_baba2.i, _baba2.j))) *
                     37;
  for (int8_t i = 0; i < GRID_HEIGHT; ++i) {
    for (int8_t j = 0; j < GRID_WIDTH; ++j) {
      hash = ApplyHash(_grid[i][j], hash);
    }
  }
  return hash;
}

bool GameStateMtn6::Equals(const GameState& other) const {
  if (const GameStateMtn6* rhs = dynamic_cast<const GameStateMtn6*>(&other)) {
    if (_baba1.i != rhs->_baba1.i || _baba1.j != rhs->_baba1.j) return false;
    if (_baba2.i != rhs->_baba2.i || _baba2.j != rhs->_baba2.j) return false;
    for (int8_t i = 0; i < GRID_HEIGHT; ++i) {
      for (int8_t j = 0; j < GRID_WIDTH; ++j) {
        if (_grid[i][j] != rhs->_grid[i][j]) return false;
      }
    }
    return true;
  }
  return false;
}

uint8_t GameStateMtn6::TurnCount() const { return _turn; }

std::vector<Direction> GameStateMtn6::Moves() const {
  return std::vector<Direction>(_moves, _moves + _turn);
}

std::shared_ptr<GameState> GameStateMtn6::ApplyMove(Direction direction) const {
  std::shared_ptr<GameStateMtn6> new_state =
      std::make_shared<GameStateMtn6>(*this);
  new_state->MoveBaba(new_state->_baba1, direction);
  new_state->MoveBaba(new_state->_baba2, direction);
  new_state->RecalculateState();
  new_state->_moves[new_state->_turn] = direction;
  new_state->_turn += 1;
  return new_state;
}

bool GameStateMtn6::HaveWon() const {
  // If the Babas are alive *and* the door and key are on the same space, we've
  // won. We technically need a Baba to touch the flag that's unlocked by
  // putting the key and the door in the same space, but for simplicity I've
  // removed this requirement.
  return AllBabasAlive() && _door.i == _key.i && _door.j == _key.j;
}

bool GameStateMtn6::CheckIfPossibleToWin() const {
  if (!AllBabasAlive()) return false;

  int rock_count = 0;
  for (int8_t j = 5; j <= 9; ++j) {
    if (CellContainsGameObject(_grid[3][j], GameObject::ROCK) ||
        CellContainsGameObject(_grid[10][j], GameObject::ROCK) ||
        CellContainsGameObject(_grid[9][j], GameObject::DOOR) ||
        CellContainsGameObject(_grid[15][j], GameObject::DOOR))
      return false;
    if (CellContainsGameObject(_grid[8][j], GameObject::ROCK)) ++rock_count;
  }
  if (rock_count >= 2) return false;

  for (int8_t j = 13; j <= 17; ++j) {
    if (CellContainsGameObjects(_grid[3][j], ROCK_AND_TEXT_BITMASK) ||
        CellContainsGameObject(_grid[9][j], GameObject::ROCK) ||
        CellContainsGameObject(_grid[8][j], GameObject::KEY) ||
        CellContainsGameObjects(_grid[14][j], KEY_AND_TEXT_BITMASK))
      return false;

    if (CellContainsGameObjects(_grid[8][j], TEXT_BITMASK) &&
        !CellContainsGameObject(_grid[8][j], GameObject::ROCK))
      return false;

    if (CellContainsGameObjects(_grid[9][j], TEXT_BITMASK) &&
        !CellContainsGameObject(_grid[8][j], GameObject::ROCK))
      return false;
  }
  for (int8_t i = 3; i <= 7; ++i) {
    if (CellContainsGameObject(_grid[i][5], GameObject::ROCK) ||
        CellContainsGameObject(_grid[i][9], GameObject::ROCK) ||
        CellContainsGameObjects(_grid[i][13], ROCK_AND_TEXT_BITMASK) ||
        CellContainsGameObjects(_grid[i][17], ROCK_AND_TEXT_BITMASK))
      return false;
  }
  for (int8_t i = 10; i <= 14; ++i) {
    if (CellContainsGameObject(_grid[i][4], GameObject::DOOR) ||
        CellContainsGameObject(_grid[i][10], GameObject::DOOR))
      return false;
  }
  for (int8_t i = 9; i <= 13; ++i) {
    if (CellContainsGameObjects(_grid[i][17], KEY_AND_TEXT_BITMASK))
      return false;
  }

  if (CellContainsGameObjects(_grid[9][12], KEY_AND_TEXT_BITMASK) ||
      CellContainsGameObjects(_grid[9][13], KEY_AND_TEXT_BITMASK))
    return false;

  if (_key.j >= 13) {
    for (int8_t i = 10; i <= 13; ++i) {
      if (CellContainsGameObjects(_grid[i][13], TEXT_BITMASK)) return false;
    }
  }

  // If the door and key are on the same row and are exactly 8 spaces apart (the
  // same width that the Babas are apart), then there's no way to move the key
  // without moving the door as well, except if you use a text block to move the
  // key. While this is technically not an unwinnable game state, it's not a
  // good state to be in, so we consider it "unwinnable" so that that part of
  // the move tree gets pruned.
  if (_door.i == _key.i && _door.j + 8 == _key.j) return false;
  return true;
}

int GameStateMtn6::CalculateScore() const {
  if (!CheckIfPossibleToWin()) return -1'000'000;

  // First milestone: Create the "rock bridges".
  int score = 0;
  bool first_milestone = false;
  for (int8_t j = 5; j <= 9; ++j) {
    if (CellContainsGameObject(_grid[8][j], GameObject::ROCK) &&
        CellContainsGameObject(_grid[9][j], GameObject::ROCK) &&
        CellContainsGameObject(_grid[8][j + 8], GameObject::ROCK)) {
      score += 1000;
      first_milestone = true;
      break;
    }
  }

  // Second milestone: Have all text blocks in the lower right quandrant.
  int8_t rows[3]{};
  int idx = 0;
  if (first_milestone) {
    for (int8_t i = 9; i <= 13; ++i) {
      for (int8_t j = 13; j <= 17; ++j) {
        if (CellContainsGameObjects(_grid[i][j], TEXT_BITMASK)) {
          score += 10'000;
          rows[idx++] = i;
        }
      }
    }
  }

  // Third milestone: Have the text blocks lined up with the key.
  if (idx >= 3) {
    // In milestone 3, if Baba2 goes back to the upper right quandrant, then
    // penalize the game state.
    if (_baba2.i <= 8) return -1'000'000;

    int8_t row = rows[0];
    bool same_row = true;
    for (int i = 1; i < 3; ++i) {
      if (rows[i] != row) {
        same_row = false;
        break;
      }
    }
    if (same_row && row == _key.i && row != _door.i) {
      score += 100'000;
    }
  }

  int distance = std::abs(_key.i - _door.i) + std::abs(_key.j - _door.j);
  score += (100 - distance);
  return score;
}

void GameStateMtn6::PrintGrid() const {
  GameObject objects_by_priority[] = {
      GameObject::IMMOVABLE, GameObject::KEY,     GameObject::DOOR,
      GameObject::PUSH_TEXT, GameObject::IS_TEXT, GameObject::ROCK_TEXT,
      GameObject::ROCK,      GameObject::TILE,
  };
  std::string perimeter(GRID_WIDTH + 2, 'X');
  std::println("{}", perimeter);
  for (int8_t i = 0; i < GRID_HEIGHT; ++i) {
    std::print("X");
    for (int8_t j = 0; j < GRID_WIDTH; ++j) {
      if ((i == _baba1.i && j == _baba1.j) ||
          (i == _baba2.i && j == _baba2.j)) {
        std::print("B");
        continue;
      }

      bool found_obj = false;
      for (GameObject obj : objects_by_priority) {
        if (CellContainsGameObject(_grid[i][j], obj)) {
          if (obj == GameObject::KEY &&
              CellContainsGameObject(_grid[i][j], GameObject::DOOR)) {
            // The key and the door are on the same space. Print 'F' for "flag".
            std::print("F");
          } else {
            std::print("{}", GameObjectToChar(obj));
          }
          found_obj = true;
          break;
        }
      }
      if (found_obj) continue;

      if (CellIsEmpty(_grid[i][j])) {
        std::print(" ");
      } else {
        // Programmer error
        std::println(stderr, "Unable to print GameState grid cell: {}",
                     _grid[i][j]);
        std::abort();
      }
    }
    std::println("X");
  }
  std::println("{}", perimeter);
}

void GameStateMtn6::PrintMoves() const {
  std::print("{:2} moves:", static_cast<uint32_t>(_turn));
  for (int i = 0; i < _turn; ++i) {
    switch (_moves[i]) {
      case Direction::UP:
        std::print(" U");
        break;
      case Direction::RIGHT:
        std::print(" R");
        break;
      case Direction::DOWN:
        std::print(" D");
        break;
      case Direction::LEFT:
        std::print(" L");
        break;
      default:
        // Should not be able to reach this code.
        std::println(stderr, "Invalid direction in GameState::PrintMoves(): {}",
                     static_cast<uint32_t>(_moves[i]));
        std::abort();
    }
  }
  std::println();
}

void GameStateMtn6::ResetContext() {
  _turn = 0;
  for (int i = 0; i < MAX_TURN_COUNT; ++i) {
    _moves[i] = Direction::NO_DIRECTION;
  }
}

void GameStateMtn6::MoveBaba(Coordinate& baba, Direction direction) {
  if (baba.i == BABA_DEAD) return;

  int8_t delta_i = 0;
  int8_t delta_j = 0;
  switch (direction) {
    case Direction::UP:
      if (baba.i == 0) return;
      delta_i = -1;
      break;
    case Direction::RIGHT:
      if (baba.j == GRID_WIDTH - 1) return;
      delta_j = 1;
      break;
    case Direction::DOWN:
      if (baba.i == GRID_HEIGHT - 1) return;
      delta_i = 1;
      break;
    case Direction::LEFT:
      if (baba.j == 0) return;
      delta_j = -1;
      break;
    default:
      // Should not be able to reach this code.
      std::println(stderr, "Invalid direction in GameState::MoveBaba(): {}",
                   static_cast<uint32_t>(direction));
      std::abort();
  }

  // Check if the cell that the Baba wants to move to is open, and, if so, move
  // all the objects in that cell. If Baba is on the same space as the key
  // (somehow), the Baba won't actually move the key. Setting prev_cell to 0
  // prevents the key from being moved.
  bool can_move = CheckCellAndMoveObjects(baba.i + delta_i, baba.j + delta_j,
                                          delta_i, delta_j, 0);
  if (!can_move) return;
  baba.i += delta_i;
  baba.j += delta_j;
}

bool GameStateMtn6::CheckCellAndMoveObjects(int8_t i, int8_t j, int8_t delta_i,
                                            int8_t delta_j,
                                            uint16_t prev_cell) {
  uint16_t cell = _grid[i][j];
  if (CellContainsGameObject(cell, GameObject::IMMOVABLE)) return false;
  // Edge case: If the key is the only movable object in the previous cell and
  // the current cell contains the door, then we can push the key into the door
  // (which is how we win).
  if (CellContainsGameObject(cell, GameObject::DOOR) &&
      CellContainsGameObject(prev_cell, GameObject::KEY)) {
    uint16_t prev_cell_without_key = RemoveFromCell(prev_cell, GameObject::KEY);
    if (!CellContainsMovableObject(prev_cell_without_key, _rock_is_push_active))
      return true;
  }
  if (!CellContainsMovableObject(cell, _rock_is_push_active)) return true;

  // There is a movable object in the current cell. We need to check the next
  // cell to see if there is space for the object to move.
  if (WillGoOutOfBounds(i, j, delta_i, delta_j)) return false;

  int8_t next_i = i + delta_i;
  int8_t next_j = j + delta_j;
  bool can_move =
      CheckCellAndMoveObjects(next_i, next_j, delta_i, delta_j, _grid[i][j]);
  if (!can_move) return false;

  // We've confirmed that the Baba can move. Now we need to move objects from
  // the current cell to the next cell.
  for (GameObject obj : ALWAYS_MOVABLE_OBJECTS) {
    if (!CellContainsGameObject(_grid[i][j], obj)) continue;
    RemoveFromCellInPlace(_grid[i][j], obj);
    AddToCellInPlace(_grid[next_i][next_j], obj);
    if (obj == GameObject::IS_TEXT) {
      _is_text.i = next_i;
      _is_text.j = next_j;
    } else if (obj == GameObject::KEY) {
      _key.i = next_i;
      _key.j = next_j;
    } else if (obj == GameObject::DOOR) {
      _door.i = next_i;
      _door.j = next_j;
    }
  }
  if (_rock_is_push_active &&
      CellContainsGameObject(_grid[i][j], GameObject::ROCK)) {
    RemoveFromCellInPlace(_grid[i][j], GameObject::ROCK);
    AddToCellInPlace(_grid[next_i][next_j], GameObject::ROCK);
  }
  return true;
}

void GameStateMtn6::RecalculateState() {
  // Check if any of the Babas are dead.
  if (!BabasOnSameSpace()) {
    if (_baba1.i != BABA_DEAD && CellIsEmpty(_grid[_baba1.i][_baba1.j])) {
      _baba1.i = BABA_DEAD;
      _baba1.j = BABA_DEAD;
    }
    if (_baba2.i != BABA_DEAD && CellIsEmpty(_grid[_baba2.i][_baba2.j])) {
      _baba2.i = BABA_DEAD;
      _baba2.j = BABA_DEAD;
    }
  }

  // Check if the "rock is push" rule is active.
  _rock_is_push_active = CheckRockIsPushIntact();
}

bool GameStateMtn6::WillGoOutOfBounds(int8_t i, int8_t j, int8_t delta_i,
                                      int8_t delta_j) const {
  return (i == 0 && delta_i < 0) || (i == GRID_HEIGHT - 1 && delta_i > 0) ||
         (j == 0 && delta_j < 0) || (j == GRID_WIDTH - 1 && delta_j > 0);
}

bool GameStateMtn6::AllBabasAlive() const {
  return _baba1.i != BABA_DEAD && _baba2.i != BABA_DEAD;
}

bool GameStateMtn6::BabasOnSameSpace() const {
  if (_baba1.i == BABA_DEAD || _baba2.i == BABA_DEAD) return false;
  return _baba1.i == _baba2.i && _baba1.j == _baba2.j;
}

bool GameStateMtn6::CheckRockIsPushIntact() const {
  if (_is_text.i > 0 && _is_text.i < GRID_HEIGHT - 1) {
    // Check "rock is push" rule vertically.
    if (CellContainsGameObject(_grid[_is_text.i - 1][_is_text.j],
                               GameObject::ROCK_TEXT) &&
        CellContainsGameObject(_grid[_is_text.i + 1][_is_text.j],
                               GameObject::PUSH_TEXT)) {
      return true;
    }
  }
  if (_is_text.j > 0 && _is_text.j < GRID_WIDTH - 1) {
    // Check "rock is push" rule horizontally.
    if (CellContainsGameObject(_grid[_is_text.i][_is_text.j - 1],
                               GameObject::ROCK_TEXT) &&
        CellContainsGameObject(_grid[_is_text.i][_is_text.j + 1],
                               GameObject::PUSH_TEXT)) {
      return true;
    }
  }
  return false;
}

}  // namespace BabaSolver
