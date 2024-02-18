// BabaSolver solves the level "The Floatiest Platforms" in the game Baba Is You.
//
// The algorithm is a brute force algorithm, calculating all the moves you can do and printing out
// the first one that succeeds in beating the level.
//
// For best performance, run the program in "release" mode, not "debug" mode.

// TODO:
// * Implement "rock is push" rule, including breaking the rule. DONE
// * Allow Baba to die if it falls off the platform. DONE
// * Implement win condition. (Shortcut: Key touching door wins.) DONE
// * Add heuristic for preventing back-and-forth moves. OBSOLETE (see next item)
// * Prune sub-trees based on if we've already calculated them. DONE
// * Get rid of outer perimeter of immovable objects. DONE
// * Parallelize the algorithm.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#define TESTING 0

// Tuning parameters - use these to trade off CPU usage, memory usage, and time to complete.
static constexpr uint16_t MAX_TURN_DEPTH = 20;
static constexpr uint16_t CACHED_MOVES_MAX_TURN_DEPTH = 20;
static constexpr uint64_t PRINT_EVERY_N_MOVES = 100'000;

// Sentinel values
static constexpr uint8_t BABA_DEAD = std::numeric_limits<uint8_t>::max();

// Level-specific constants
static constexpr uint8_t GRID_HEIGHT = 18;
static constexpr uint8_t GRID_WIDTH = 18;
static constexpr uint16_t NUM_CELLS = GRID_HEIGHT * GRID_WIDTH;
static constexpr uint8_t IS_TEXT_START_I = 4;
static constexpr uint8_t IS_TEXT_START_J = 12;
#if TESTING
static constexpr uint8_t DOOR_I = 5;
static constexpr uint8_t DOOR_J = 2;
#else
static constexpr uint8_t DOOR_I = 12;
static constexpr uint8_t DOOR_J = 4;
#endif

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

static char GameObjectToChar(GameObject obj)
{
	switch (obj)
	{
	case GameObject::BABA: return 'B';
	case GameObject::IMMOVABLE: return 'X';
	case GameObject::TILE: return '^';
	case GameObject::ROCK: return 'R';
	case GameObject::DOOR: return 'D';
	case GameObject::KEY: return 'K';
	case GameObject::ROCK_TEXT: return '1';
	case GameObject::IS_TEXT: return '2';
	case GameObject::PUSH_TEXT: return '3';
	}
	throw "Invalid GameObject";
}

// Note: This does NOT check for Babas in the cell.
static bool CellIsEmpty(uint16_t cell)
{
	return cell == 0;
}

static bool CellContainsGameObject(uint16_t cell, GameObject obj)
{
	uint16_t bitmask = 1 << static_cast<uint16_t>(obj);
	return (cell & bitmask) != 0;
}

static constexpr uint16_t IMMOVABLE_OBJECT_BITMASK = (1 << static_cast<uint16_t>(GameObject::IMMOVABLE)) |
	(1 << static_cast<uint16_t>(GameObject::DOOR));

static bool CellContainsImmovableObject(uint16_t cell)
{
	return (cell & IMMOVABLE_OBJECT_BITMASK) != 0;
}

static constexpr uint16_t MOVABLE_OBJECT_BITMASK = (1 << static_cast<uint16_t>(GameObject::KEY)) |
	(1 << static_cast<uint16_t>(GameObject::ROCK_TEXT)) | (1 << static_cast<uint16_t>(GameObject::IS_TEXT)) |
	(1 << static_cast<uint16_t>(GameObject::PUSH_TEXT));

static bool CellContainsMovableObject(uint16_t cell, bool rock_is_push_active)
{
	if ((cell & MOVABLE_OBJECT_BITMASK) != 0)
		return true;
	if (rock_is_push_active && CellContainsGameObject(cell, GameObject::ROCK))
		return true;
	return false;
}

static void AddToCellInPlace(uint16_t& cell, GameObject obj)
{
	uint16_t bitmask = 1 << static_cast<uint16_t>(obj);
	cell |= bitmask;
}

static uint16_t AddToCell(uint16_t cell, GameObject obj)
{
	AddToCellInPlace(cell, obj);
	return cell;
}

static void RemoveFromCellInPlace(uint16_t& cell, GameObject obj)
{
	uint16_t bitmask = ~(1 << static_cast<uint16_t>(obj));
	cell &= bitmask;
}

static uint16_t RemoveFromCell(uint16_t cell, GameObject obj)
{
	RemoveFromCellInPlace(cell, obj);
	return cell;
}

static GameObject ALWAYS_MOVABLE_OBJECTS[] = {
	GameObject::KEY, GameObject::ROCK_TEXT, GameObject::IS_TEXT, GameObject::PUSH_TEXT };

class GameState
{
public:
	uint8_t _turn;
	uint16_t _grid[GRID_HEIGHT][GRID_WIDTH];
	Direction _moves[MAX_TURN_DEPTH];
	Coordinate _baba1;
	Coordinate _baba2;
	bool _won;

private:
	Coordinate _is_text;
	bool _rock_is_push_active;

public:
	GameState(Coordinate baba1, Coordinate baba2, Coordinate is_text, bool rock_is_push_active)
		: _turn(0), _won(false), _baba1(baba1), _baba2(baba2), _is_text(is_text), _rock_is_push_active(rock_is_push_active) {
		for (uint8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (uint8_t j = 0; j < GRID_WIDTH; ++j)
			{
				_grid[i][j] = 0;
			}
		}
		for (uint16_t i = 0; i < MAX_TURN_DEPTH; ++i)
		{
			_moves[i] = Direction::NO_DIRECTION;
		}
	}

	GameState(uint8_t turn, uint16_t grid[GRID_HEIGHT][GRID_WIDTH], Direction moves[MAX_TURN_DEPTH],
		Coordinate baba1, Coordinate baba2, Coordinate is_text, bool rock_is_push_active)
		: _turn(turn), _won(false), _baba1(baba1), _baba2(baba2), _is_text(is_text), _rock_is_push_active(rock_is_push_active) {
		std::copy(&grid[0][0], &grid[0][0] + NUM_CELLS, &_grid[0][0]);
		std::copy(&moves[0], &moves[0] + MAX_TURN_DEPTH, &_moves[0]);
	}

	std::shared_ptr<GameState> ApplyMove(Direction direction)
	{
		std::shared_ptr<GameState> new_state = std::make_shared<GameState>(
			_turn+1, _grid, _moves, _baba1, _baba2, _is_text, _rock_is_push_active);
		new_state->MoveBaba(new_state->_baba1, direction);
		new_state->MoveBaba(new_state->_baba2, direction);
		new_state->RecalculateState();
		new_state->_moves[new_state->_turn - 1] = direction;
		return new_state;
	}

	bool AllBabasAlive()
	{
		return _baba1.i != BABA_DEAD && _baba2.i != BABA_DEAD;
	}

	bool BabasOnSameSpace()
	{
		if (_baba1.i == BABA_DEAD || _baba2.i == BABA_DEAD)
			return false;
		return _baba1.i == _baba2.i && _baba1.j == _baba2.j;
	}

	void Print()
	{
		GameObject objects_by_priority[] = {
				GameObject::ROCK_TEXT,
				GameObject::IS_TEXT,
				GameObject::PUSH_TEXT,
				GameObject::DOOR,
				GameObject::KEY,
				GameObject::ROCK,
				GameObject::IMMOVABLE,
				GameObject::TILE,
		};
		std::string perimeter(GRID_WIDTH + 2, 'X');
		std::cout << perimeter << '\n';
		for (uint8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			std::cout << 'X';
			for (uint8_t j = 0; j < GRID_WIDTH; ++j)
			{
				if ((i == _baba1.i && j == _baba1.j) || (i == _baba2.i && j == _baba2.j))
				{
					std::cout << 'B';
					continue;
				}
				if (CellIsEmpty(_grid[i][j]))
				{
					std::cout << ' ';
					continue;
				}

				for (GameObject obj : objects_by_priority)
				{
					if (CellContainsGameObject(_grid[i][j], obj))
					{
						std::cout << GameObjectToChar(obj);
						break;
					}
				}
			}
			std::cout << "X\n";
		}
		std::cout << perimeter << std::endl;
	}

private:
	void MoveBaba(Coordinate& baba, Direction direction)
	{
		if (baba.i == BABA_DEAD)
			return;

		int8_t delta_i = 0;
		int8_t delta_j = 0;
		switch (direction)
		{
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
			throw "Invalid direction";
		}

		// If Baba is on the same space as the KEY (somehow), the Baba won't actually move the KEY.
		// Setting prev_cell to 0 prevents the KEY from being moved.
		bool can_move = CheckCellAndMoveObjects(baba.i + delta_i, baba.j + delta_j, delta_i, delta_j, 0);
		if (!can_move)
			return;
		baba.i += delta_i;
		baba.j += delta_j;
	}

	bool CheckCellAndMoveObjects(uint8_t i, uint8_t j, int8_t delta_i, int8_t delta_j, uint16_t prev_cell)
	{
		uint16_t cell = _grid[i][j];
		if (CellContainsGameObject(cell, GameObject::IMMOVABLE))
			return false;
		// Edge case: If the KEY is the only movable object in the previous cell and the current
		// cell contains the DOOR, then we can push the KEY into the DOOR (which is how we win).
		if (CellContainsGameObject(cell, GameObject::DOOR))
		{
			if (CellContainsGameObject(prev_cell, GameObject::KEY))
			{
				uint16_t prev_cell_without_key = RemoveFromCell(prev_cell, GameObject::KEY);
				if (!CellContainsMovableObject(prev_cell_without_key, _rock_is_push_active))
					return true;
			}
			return false;
		}
		if (!CellContainsMovableObject(cell, _rock_is_push_active))
			return true;

		// There is a movable object in the current cell. We need to check the next cell to see if
		// there is space for the object to move.
		if (WillGoOutOfBounds(i, j, delta_i, delta_j))
			return false;
		bool can_move = CheckCellAndMoveObjects(i + delta_i, j + delta_j, delta_i, delta_j, _grid[i][j]);
		if (!can_move)
			return false;

		// We've confirmed that the Baba can move. Now we need to move objects from the current
		// cell to the next cell.
		for (GameObject obj : ALWAYS_MOVABLE_OBJECTS)
		{
			if (!CellContainsGameObject(_grid[i][j], obj))
				continue;
			RemoveFromCellInPlace(_grid[i][j], obj);
			AddToCellInPlace(_grid[i + delta_i][j + delta_j], obj);
			if (obj == GameObject::IS_TEXT)
			{
				_is_text.i = i + delta_i;
				_is_text.j = j + delta_j;
			}
		}
		if (_rock_is_push_active && CellContainsGameObject(cell, GameObject::ROCK))
		{
			RemoveFromCellInPlace(_grid[i][j], GameObject::ROCK);
			AddToCellInPlace(_grid[i + delta_i][j + delta_j], GameObject::ROCK);
		}
		return true;
	}

	bool WillGoOutOfBounds(uint8_t i, uint8_t j, int8_t delta_i, int8_t delta_j)
	{
		return (i == 0 && delta_i < 0) || (i == GRID_HEIGHT - 1 && delta_i > 0) ||
			(j == 0 && delta_j < 0) || (j == GRID_WIDTH - 1 && delta_j > 0);
	}

	void RecalculateState()
	{
		// Check if we won.
		// Optimization: The Door's coordinates are hard-coded because it doesn't move.
		if (CellContainsGameObject(_grid[DOOR_I][DOOR_J], GameObject::KEY))
			_won = true;

		// Check if any of the Babas are dead.
		if (!BabasOnSameSpace())
		{
			if (_baba1.i != BABA_DEAD && CellIsEmpty(_grid[_baba1.i][_baba1.j]))
			{
				_baba1.i = BABA_DEAD;
				_baba1.j = BABA_DEAD;
			}
			if (_baba2.i != BABA_DEAD && CellIsEmpty(_grid[_baba2.i][_baba2.j]))
			{
				_baba2.i = BABA_DEAD;
				_baba2.j = BABA_DEAD;
			}
		}

		// Check if the "rock is push" rule is active.
		_rock_is_push_active = CheckRockIsPushIntact();
	}

	bool CheckRockIsPushIntact()
	{
		if (_is_text.i > 0 && _is_text.i < GRID_HEIGHT - 1)
		{
			// Check "rock is push" rule vertically.
			if (CellContainsGameObject(_grid[_is_text.i - 1][_is_text.j], GameObject::ROCK_TEXT)
				&& CellContainsGameObject(_grid[_is_text.i + 1][_is_text.j], GameObject::PUSH_TEXT))
			{
				return true;
			}
		}
		if (_is_text.j > 0 && _is_text.j < GRID_WIDTH - 1)
		{
			// Check "rock is push" rule horizontally.
			if (CellContainsGameObject(_grid[_is_text.i][_is_text.j - 1], GameObject::ROCK_TEXT)
				&& CellContainsGameObject(_grid[_is_text.i][_is_text.j + 1], GameObject::PUSH_TEXT))
			{
				return true;
			}
		}
		return false;
	}
};

struct NextMove
{
	std::shared_ptr<GameState> state;
	Direction dir_to_apply;
};

struct NextMoveEqual
{
	bool operator()(const NextMove& lhs, const NextMove& rhs) const
	{
		if (lhs.dir_to_apply != rhs.dir_to_apply) return false;
		// Turn number is checked because the remaining number of turns can affect the final
		// outcome of the GameState.
		if (lhs.state->_turn != rhs.state->_turn) return false;
		if (lhs.state->_baba1.i != rhs.state->_baba1.i || lhs.state->_baba1.j != rhs.state->_baba1.j) return false;
		if (lhs.state->_baba2.i != rhs.state->_baba2.i || lhs.state->_baba2.j != rhs.state->_baba2.j) return false;
		for (uint8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (uint8_t j = 0; j < GRID_WIDTH; ++j)
			{
				if (lhs.state->_grid[i][j] != rhs.state->_grid[i][j])
					return false;
			}
		}
		return true;
	}
};

static std::size_t ApplyHash(uint32_t to_apply, std::size_t current_hash)
{
	// Note: I got this code from Stack Overflow somewhere. I have no idea how good or bad it is.
	// TODO: Check how effective this hashing code is.
	to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
	to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
	to_apply = (to_apply >> 16) ^ to_apply;
	current_hash ^= static_cast<std::size_t>(to_apply) + 0x9e3779b9 + (current_hash << 6) + (current_hash >> 2);
	return current_hash;
}

static uint16_t CombineUInt8s(uint8_t n1, uint8_t n2)
{
	return (static_cast<uint16_t>(n1) << 8) | n2;
}

static uint32_t CombineUInt16s(uint16_t n1, uint16_t n2)
{
	return (static_cast<uint32_t>(n1) << 16) | n2;
}

static uint64_t CombineUInt32s(uint32_t n1, uint32_t n2)
{
	return (static_cast<uint64_t>(n1) << 32) | n2;
}

struct NextMoveHash
{
	std::size_t operator()(const NextMove& move) const
	{
		std::size_t hash = CombineUInt16s(CombineUInt8s(move.state->_baba1.i, move.state->_baba1.j),
			CombineUInt8s(move.state->_baba2.i, move.state->_baba2.j));
		uint16_t a = CombineUInt8s(static_cast<uint8_t>(move.dir_to_apply), move.state->_turn);
		hash = ApplyHash(static_cast<uint32_t>(a), hash);
		for (uint8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (uint8_t j = 0; j < GRID_WIDTH; ++j)
			{
				hash = ApplyHash(move.state->_grid[i][j], hash);
			}
		}
		return hash;
	}
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

#if TESTING
std::shared_ptr<GameState> TestLevel()
{
	std::shared_ptr<GameState> initial_state = std::make_shared<GameState>(
		Coordinate{ 5, 4 }, Coordinate{ 5, 12 }, Coordinate{ IS_TEXT_START_I, IS_TEXT_START_J }, true);
	// Add tiles to grid.
	for (uint8_t i = 3; i <= 7; ++i)
	{
		for (uint8_t j = 2; j <= 6; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 3; i <= 7; ++i)
	{
		for (uint8_t j = 10; j <= 14; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 10; i <= 14; ++i)
	{
		for (uint8_t j = 2; j <= 6; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 9; i <= 13; ++i)
	{
		for (uint8_t j = 10; j <= 14; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	// Add the other game objects to the grid.
	AddToCellInPlace(initial_state->_grid[DOOR_I][DOOR_J], GameObject::DOOR);
	AddToCellInPlace(initial_state->_grid[5][3], GameObject::KEY);

	return initial_state;
}

#else
std::shared_ptr<GameState> FloatiestPlatformsLevel()
{
	std::shared_ptr<GameState> initial_state = std::make_shared<GameState>(
		Coordinate{ 5, 4 }, Coordinate{ 5, 12 }, Coordinate{ IS_TEXT_START_I, IS_TEXT_START_J }, true);
	// Add tiles to grid.
	for (uint8_t i = 3; i <= 7; ++i)
	{
		for (uint8_t j = 2; j <= 6; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 3; i <= 7; ++i)
	{
		for (uint8_t j = 10; j <= 14; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 10; i <= 14; ++i)
	{
		for (uint8_t j = 2; j <= 6; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	for (uint8_t i = 9; i <= 13; ++i)
	{
		for (uint8_t j = 10; j <= 14; ++j)
		{
			AddToCellInPlace(initial_state->_grid[i][j], GameObject::TILE);
		}
	}
	// Add an immovable for each text block around the corners of the grid.
	AddToCellInPlace(initial_state->_grid[0][0], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[0][1], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[0][2], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[0][7], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[0][8], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[0][9], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[16][0], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[16][1], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][0], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][1], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][2], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][3], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[15][15], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[15][16], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[15][17], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[16][15], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[16][16], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[16][17], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][15], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][16], GameObject::IMMOVABLE);
	AddToCellInPlace(initial_state->_grid[17][17], GameObject::IMMOVABLE);
	// Add the other game objects to the grid.
	AddToCellInPlace(initial_state->_grid[4][3], GameObject::ROCK);
	AddToCellInPlace(initial_state->_grid[6][5], GameObject::ROCK);
	AddToCellInPlace(initial_state->_grid[6][11], GameObject::ROCK);
	AddToCellInPlace(initial_state->_grid[4][11], GameObject::ROCK_TEXT);
	AddToCellInPlace(initial_state->_grid[IS_TEXT_START_I][IS_TEXT_START_J], GameObject::IS_TEXT);
	AddToCellInPlace(initial_state->_grid[4][13], GameObject::PUSH_TEXT);
	AddToCellInPlace(initial_state->_grid[16][2], GameObject::PUSH_TEXT);
	AddToCellInPlace(initial_state->_grid[DOOR_I][DOOR_J], GameObject::DOOR);
	AddToCellInPlace(initial_state->_grid[11][12], GameObject::KEY);

	return initial_state;
}
#endif

int main()
{
	std::cout << "Baba Is You solver" << std::endl;

#if TESTING
	std::shared_ptr<GameState> initial_state = TestLevel();
#else
	std::shared_ptr<GameState> initial_state = FloatiestPlatformsLevel();
#endif

	initial_state->Print();

	std::stack<NextMove> stack;
	// Apply initial four directions to the stack.
	stack.push(NextMove{ initial_state, Direction::UP });
	stack.push(NextMove{ initial_state, Direction::RIGHT });
	stack.push(NextMove{ initial_state, Direction::DOWN });
	stack.push(NextMove{ initial_state, Direction::LEFT });
	initial_state.reset();

	std::unordered_set<NextMove, NextMoveHash, NextMoveEqual> computed_moves;

	uint64_t num_moves = 0;
	uint64_t num_cache_hits = 0;
	bool won = false;
	bool found_invincible_babas_trick = false;
	auto start_time = std::chrono::high_resolution_clock::now();
	while (!stack.empty())
	{
		++num_moves;
		if (num_moves % PRINT_EVERY_N_MOVES == 0)
		{
			std::cout << "Calculating move #" << num_moves << " (" << FormatNumberWithSuffix(num_moves)
				<< "), cache size = " << computed_moves.size() << " (" << FormatNumberWithSuffix(computed_moves.size())
				<< "), stack size = " << stack.size() << std::endl;
		}

		const NextMove& cur = stack.top();
		if (computed_moves.find(cur) != computed_moves.end())
		{
			++num_cache_hits;
			stack.pop();
			continue;
		}
		std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
		if (new_state->_won)
		{
			won = true;
			std::cout << "WIN!!!\n";
			new_state->Print();
			break;
		}
		// Heuristic: If the Babas are on the same space, then we're close to winning and can end
		// the search here.
		if (new_state->BabasOnSameSpace())
		{
			found_invincible_babas_trick = true;
			std::cout << "Found invincible Babas trick!!! Move #" << num_moves << ", turn #" << new_state->_turn << std::endl;
			new_state->Print();
			break;
		}

		if (new_state->_turn <= CACHED_MOVES_MAX_TURN_DEPTH)
		{
			computed_moves.insert(cur);
		}
		stack.pop();

		// Heuristic: If one of the Babas dies, then prune that part of the tree.
		if (!new_state->AllBabasAlive() || new_state->_turn >= MAX_TURN_DEPTH)
		{
#if TESTING
			new_state->Print();
#endif
			continue;
		}
		stack.push(NextMove{ new_state, Direction::UP });
		stack.push(NextMove{ new_state, Direction::RIGHT });
		stack.push(NextMove{ new_state, Direction::DOWN });
		stack.push(NextMove{ new_state, Direction::LEFT });
		new_state.reset();
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	auto total_time = end_time - start_time;

	if (!won)
	{
		std::cout << "Did not win...\n";
	}
	if (!found_invincible_babas_trick)
	{
		std::cout << "Did not find the invincible Babas trick...\n";
	}
	std::cout << "Max move depth: " << MAX_TURN_DEPTH << "\n";
	std::cout << "Max cache depth: " << CACHED_MOVES_MAX_TURN_DEPTH << "\n";
	std::cout << "Total number of moves simulated (including cache hits): " << FormatNumberWithCommas(num_moves) << "\n";
	std::cout << "Cache size: " << FormatNumberWithCommas(computed_moves.size()) << " moves\n";
	std::cout << "Number of cache hits: " << FormatNumberWithCommas(num_cache_hits) << "\n";
	std::cout << "Number of unique, non-cached moves: " << FormatNumberWithCommas(num_moves - num_cache_hits) << "\n";
	std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::seconds>(total_time).count() << " seconds\n";
	std::cout << "Time per move: " << (total_time.count() / num_moves) << " nanoseconds" << std::endl;
}
