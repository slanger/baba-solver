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

namespace BabaSolver
{
	static constexpr int8_t DOOR_I = 12;
	static constexpr int8_t DOOR_J = 4;

	static constexpr int8_t BABA_DEAD = -1;

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
		// Should not be able to reach this point.
		std::cerr << "GameObject isn't set in GameObjectToChar(): " << static_cast<uint16_t>(obj) << std::endl;
		std::abort();
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

	GameState::GameState(uint16_t grid[GRID_HEIGHT][GRID_WIDTH], Coordinate baba1, Coordinate baba2)
		: _baba1(baba1), _baba2(baba2), _turn(0), _key({ -1, -1 }), _is_text({ -1, -1 }), _rock_is_push_active()
	{
		for (int8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (int8_t j = 0; j < GRID_WIDTH; ++j)
			{
				_grid[i][j] = grid[i][j];
				if (CellContainsGameObject(grid[i][j], GameObject::KEY))
				{
					_key.i = i;
					_key.j = j;
				}
				if (CellContainsGameObject(grid[i][j], GameObject::IS_TEXT))
				{
					_is_text.i = i;
					_is_text.j = j;
				}
			}
		}
		if (_key.i == -1 || _is_text.i == -1 || !CellContainsGameObject(grid[DOOR_I][DOOR_J], GameObject::DOOR))
		{
			// Programmer error
			std::cerr << "Invalid GameState" << std::endl;
			std::abort();
		}
		for (int i = 0; i < MAX_TURN_COUNT; ++i)
		{
			_moves[i] = Direction::NO_DIRECTION;
		}
		RecalculateState();
	}

	GameState::GameState(const GameState& other)
		: _baba1(other._baba1), _baba2(other._baba2), _turn(other._turn), _key(other._key), _is_text(other._is_text), _rock_is_push_active(other._rock_is_push_active)
	{
		std::copy(&other._grid[0][0], &other._grid[0][0] + GRID_CELL_COUNT, &_grid[0][0]);
		std::copy(&other._moves[0], &other._moves[0] + other._turn, &_moves[0]);
	}

	void GameState::ResetContext()
	{
		_turn = 0;
		for (int i = 0; i < MAX_TURN_COUNT; ++i)
		{
			_moves[i] = Direction::NO_DIRECTION;
		}
	}

	std::shared_ptr<GameState> GameState::ApplyMove(Direction direction) const
	{
		std::shared_ptr<GameState> new_state = std::make_shared<GameState>(*this);
		new_state->MoveBaba(new_state->_baba1, direction);
		new_state->MoveBaba(new_state->_baba2, direction);
		new_state->RecalculateState();
		new_state->_moves[new_state->_turn] = direction;
		new_state->_turn += 1;
		return new_state;
	}

	bool GameState::HaveWon() const
	{
		// Optimization: The Door's coordinates are hard-coded because it doesn't move.
		return CellContainsGameObject(_grid[DOOR_I][DOOR_J], GameObject::KEY);
	}

	bool GameState::CheckIfPossibleToWin() const
	{
		if (!AllBabasAlive())
			return false;

		// If any of the text objects are *not* on the upper right platform or to the right of it,
		// then it's impossible to win.
		if (_is_text.i <= 2 || _is_text.i >= 8 || _is_text.j <= 9)
			return false;

		for (int8_t i = 0; i <= 2; ++i)
		{
			for (int8_t j = 10; j <= 17; ++j)
			{
				if (CellContainsGameObject(_grid[i][j], GameObject::ROCK_TEXT) || CellContainsGameObject(_grid[i][j], GameObject::PUSH_TEXT))
					return false;
			}
		}
		for (int8_t i = 8; i <= 10; ++i)
		{
			for (int8_t j = 10; j <= 17; ++j)
			{
				if (CellContainsGameObject(_grid[i][j], GameObject::ROCK_TEXT) || CellContainsGameObject(_grid[i][j], GameObject::PUSH_TEXT))
					return false;
			}
		}
		for (int8_t i = 3; i <= 7; ++i)
		{
			for (int8_t j = 7; j <= 9; ++j)
			{
				if (CellContainsGameObject(_grid[i][j], GameObject::ROCK_TEXT) || CellContainsGameObject(_grid[i][j], GameObject::PUSH_TEXT))
					return false;
			}
		}
		return true;
	}

	int GameState::CalculateScore() const
	{
		if (!CheckIfPossibleToWin())
			return -1'000'000;

		// First milestone: The three rocks are moved in between the two upper platforms. This
		// creates a "bridge" between the two platforms.
		int score = 0;
		int8_t rock_row = -1;
		for (int8_t i = 3; i <= 7; ++i)
		{
			int rock_count = 0;
			for (int8_t j = 7; j <= 9; ++j)
			{
				if (CellContainsGameObject(_grid[i][j], GameObject::ROCK))
					++rock_count;
			}
			switch (rock_count)
			{
			case 1:
				score += 100;
				break;
			case 2:
				score += 1'000;
				break;
			case 3:
				score += 10'000;
				rock_row = i;
				break;
			default:
				break;
			}
		}

		// Second milestone: The text blocks are moved to the right of the upper right platform and
		// are aligned with the rocks that are in between the upper platforms. This lets the player
		// move the Babas closer together (over the rocks and stopped on the right by the text
		// blocks).
		if (rock_row != -1)
		{
			if (!CheckIfTextCanBeAlignedWithRocks(rock_row))
				return -1;

			int text_aligned_count = 0;
			if (_is_text.i == rock_row)
				++text_aligned_count;
			for (int8_t j = 10; j <= 17; ++j)
			{
				if (CellContainsGameObject(_grid[rock_row][j], GameObject::ROCK_TEXT) || CellContainsGameObject(_grid[rock_row][j], GameObject::PUSH_TEXT))
					++text_aligned_count;
			}
			switch (text_aligned_count)
			{
			case 1:
				score += 1'000;
				break;
			case 2:
				score += 10'000;
				break;
			case 3:
				score += _rock_is_push_active ? 100'000 : 1'000'000;
				break;
			default:
				break;
			}
		}

		// Third milestone: Both Babas are on the same space. After this point, the player can move
		// the Babas freely and move the key to the door.
		if (BabasOnSameSpace())
		{
			score += 10'000'000;
		}

		// Tie breaker: The distance between the key and the door.
		int distance = std::abs(_key.i - DOOR_I) + std::abs(_key.j - DOOR_J);
		score += 100 - distance;
		return score;
	}

	void GameState::PrintGrid() const
	{
		GameObject objects_by_priority[] = {
				GameObject::IMMOVABLE,
				GameObject::KEY,
				GameObject::DOOR,
				GameObject::ROCK,
				GameObject::PUSH_TEXT,
				GameObject::IS_TEXT,
				GameObject::ROCK_TEXT,
				GameObject::TILE,
		};
		std::string perimeter(GRID_WIDTH + 2, 'X');
		std::cout << perimeter << '\n';
		for (int8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			std::cout << 'X';
			for (int8_t j = 0; j < GRID_WIDTH; ++j)
			{
				if ((i == _baba1.i && j == _baba1.j) || (i == _baba2.i && j == _baba2.j))
				{
					std::cout << 'B';
					continue;
				}

				bool found_obj = false;
				for (GameObject obj : objects_by_priority)
				{
					if (CellContainsGameObject(_grid[i][j], obj))
					{
						std::cout << GameObjectToChar(obj);
						found_obj = true;
						break;
					}
				}
				if (found_obj)
					continue;

				if (CellIsEmpty(_grid[i][j]))
				{
					std::cout << ' ';
				}
				else
				{
					// Programmer error
					std::cerr << "Unable to print GameState grid cell: " << _grid[i][j] << std::endl;
					std::abort();
				}
			}
			std::cout << "X\n";
		}
		std::cout << perimeter << std::endl;
	}

	void GameState::PrintMoves() const
	{
		std::cout << static_cast<uint32_t>(_turn) << " moves:";
		for (int i = 0; i < _turn; ++i)
		{
			switch (_moves[i])
			{
			case Direction::UP:
				std::cout << " U";
				break;
			case Direction::RIGHT:
				std::cout << " R";
				break;
			case Direction::DOWN:
				std::cout << " D";
				break;
			case Direction::LEFT:
				std::cout << " L";
				break;
			default:
				// Should not be able to reach this code.
				std::cerr << "Invalid direction in GameState::PrintMoves(): " << static_cast<uint32_t>(_moves[i]) << std::endl;
				std::abort();
			}
		}
		std::cout << std::endl;
	}

	void GameState::MoveBaba(Coordinate& baba, Direction direction)
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
			// Should not be able to reach this code.
			std::cerr << "Invalid direction in GameState::MoveBaba(): " << static_cast<uint32_t>(direction) << std::endl;
			std::abort();
		}

		// Check if the cell that the Baba wants to move to is open, and, if so, move all the
		// objects in that cell.
		// If Baba is on the same space as the key (somehow), the Baba won't actually move the key.
		// Setting prev_cell to 0 prevents the key from being moved.
		bool can_move = CheckCellAndMoveObjects(baba.i + delta_i, baba.j + delta_j, delta_i, delta_j, 0);
		if (!can_move)
			return;
		baba.i += delta_i;
		baba.j += delta_j;
	}

	bool GameState::CheckCellAndMoveObjects(int8_t i, int8_t j, int8_t delta_i, int8_t delta_j, uint16_t prev_cell)
	{
		uint16_t cell = _grid[i][j];
		if (CellContainsGameObject(cell, GameObject::IMMOVABLE))
			return false;
		// Edge case: If the key is the only movable object in the previous cell and the current
		// cell contains the door, then we can push the key into the door (which is how we win).
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

		int8_t next_i = i + delta_i;
		int8_t next_j = j + delta_j;
		bool can_move = CheckCellAndMoveObjects(next_i, next_j, delta_i, delta_j, _grid[i][j]);
		if (!can_move)
			return false;

		// We've confirmed that the Baba can move. Now we need to move objects from the current
		// cell to the next cell.
		for (GameObject obj : ALWAYS_MOVABLE_OBJECTS)
		{
			if (!CellContainsGameObject(_grid[i][j], obj))
				continue;
			RemoveFromCellInPlace(_grid[i][j], obj);
			AddToCellInPlace(_grid[next_i][next_j], obj);
			if (obj == GameObject::IS_TEXT)
			{
				_is_text.i = next_i;
				_is_text.j = next_j;
			}
			else if (obj == GameObject::KEY)
			{
				_key.i = next_i;
				_key.j = next_j;
			}
		}
		if (_rock_is_push_active && CellContainsGameObject(cell, GameObject::ROCK))
		{
			RemoveFromCellInPlace(_grid[i][j], GameObject::ROCK);
			AddToCellInPlace(_grid[next_i][next_j], GameObject::ROCK);
		}
		return true;
	}

	void GameState::RecalculateState()
	{
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

	bool GameState::WillGoOutOfBounds(int8_t i, int8_t j, int8_t delta_i, int8_t delta_j) const
	{
		return (i == 0 && delta_i < 0) || (i == GRID_HEIGHT - 1 && delta_i > 0) ||
			(j == 0 && delta_j < 0) || (j == GRID_WIDTH - 1 && delta_j > 0);
	}

	bool GameState::AllBabasAlive() const
	{
		return _baba1.i != BABA_DEAD && _baba2.i != BABA_DEAD;
	}

	bool GameState::BabasOnSameSpace() const
	{
		if (_baba1.i == BABA_DEAD || _baba2.i == BABA_DEAD)
			return false;
		return _baba1.i == _baba2.i && _baba1.j == _baba2.j;
	}

	bool GameState::CheckRockIsPushIntact() const
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

	bool GameState::CheckIfTextCanBeAlignedWithRocks(int8_t rock_row) const
	{
		if (_is_text.i != rock_row && _is_text.j >= 15)
			return false;
		if (_is_text.j >= 15 && _rock_is_push_active)
			return false;

		for (int8_t i = 3; i <= 7; ++i)
		{
			if (i == rock_row)
				continue;
			for (int8_t j = 15; j <= 17; ++j)
			{
				if (CellContainsGameObject(_grid[i][j], GameObject::ROCK_TEXT) || CellContainsGameObject(_grid[i][j], GameObject::PUSH_TEXT))
					return false;
			}
		}
		return true;
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

	static std::size_t ApplyHash(uint32_t to_apply, std::size_t current_hash)
	{
		// Note: I forgot where I got this code from, but it was on Stack Overflow somewhere. I have
		// no idea how good or bad it is.
		// TODO: Check how effective this hashing code is.
		to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
		to_apply = ((to_apply >> 16) ^ to_apply) * 0x45d9f3b;
		to_apply = (to_apply >> 16) ^ to_apply;
		current_hash ^= static_cast<std::size_t>(to_apply) + 0x9e3779b9 + (current_hash << 6) + (current_hash >> 2);
		return current_hash;
	}

	std::size_t GameStateHash::operator()(const std::shared_ptr<GameState>& state) const
	{
		std::size_t hash = static_cast<std::size_t>(CombineUInt16s(
			CombineUInt8s(state->_baba1.i, state->_baba1.j), CombineUInt8s(state->_baba2.i, state->_baba2.j))) * 37;
		for (int8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (int8_t j = 0; j < GRID_WIDTH; ++j)
			{
				hash = ApplyHash(state->_grid[i][j], hash);
			}
		}
		return hash;
	}

	bool GameStateEqual::operator()(const std::shared_ptr<GameState>& lhs, const std::shared_ptr<GameState>& rhs) const
	{
		if (lhs->_baba1.i != rhs->_baba1.i || lhs->_baba1.j != rhs->_baba1.j) return false;
		if (lhs->_baba2.i != rhs->_baba2.i || lhs->_baba2.j != rhs->_baba2.j) return false;
		for (int8_t i = 0; i < GRID_HEIGHT; ++i)
		{
			for (int8_t j = 0; j < GRID_WIDTH; ++j)
			{
				if (lhs->_grid[i][j] != rhs->_grid[i][j])
					return false;
			}
		}
		return true;
	}

	std::shared_ptr<GameState> FloatiestPlatformsLevel()
	{
		uint16_t grid[GRID_HEIGHT][GRID_WIDTH]{};
		// Add tiles to grid.
		for (int8_t i = 3; i <= 7; ++i)
		{
			for (int8_t j = 2; j <= 6; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 3; i <= 7; ++i)
		{
			for (int8_t j = 10; j <= 14; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 10; i <= 14; ++i)
		{
			for (int8_t j = 2; j <= 6; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 9; i <= 13; ++i)
		{
			for (int8_t j = 10; j <= 14; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		// Add an immovable for each text block around the corners of the grid.
		AddToCellInPlace(grid[0][0], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[0][1], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[0][2], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[0][7], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[0][8], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[0][9], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][0], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][1], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][2], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][0], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][1], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][2], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][3], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[15][15], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[15][16], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[15][17], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][15], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][16], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[16][17], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][15], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][16], GameObject::IMMOVABLE);
		AddToCellInPlace(grid[17][17], GameObject::IMMOVABLE);
		// Add the other game objects to the grid.
		AddToCellInPlace(grid[4][3], GameObject::ROCK);
		AddToCellInPlace(grid[6][5], GameObject::ROCK);
		AddToCellInPlace(grid[6][11], GameObject::ROCK);
		AddToCellInPlace(grid[4][11], GameObject::ROCK_TEXT);
		AddToCellInPlace(grid[4][12], GameObject::IS_TEXT);
		AddToCellInPlace(grid[4][13], GameObject::PUSH_TEXT);
		AddToCellInPlace(grid[DOOR_I][DOOR_J], GameObject::DOOR);
		AddToCellInPlace(grid[11][12], GameObject::KEY);

		Coordinate baba1{ 5, 4 };
		Coordinate baba2{ 5, 12 };
		return std::make_shared<GameState>(grid, baba1, baba2);
	}

	std::shared_ptr<GameState> TestLevel()
	{
		constexpr int8_t is_text_start_i = 4;
		constexpr int8_t is_text_start_j = 12;
		uint16_t grid[GRID_HEIGHT][GRID_WIDTH]{};
		// Add tiles to grid.
		for (int8_t i = 3; i <= 7; ++i)
		{
			for (int8_t j = 2; j <= 6; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 3; i <= 7; ++i)
		{
			for (int8_t j = 10; j <= 14; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 10; i <= 14; ++i)
		{
			for (int8_t j = 2; j <= 6; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		for (int8_t i = 9; i <= 13; ++i)
		{
			for (int8_t j = 10; j <= 14; ++j)
			{
				AddToCellInPlace(grid[i][j], GameObject::TILE);
			}
		}
		// Add the other game objects to the grid.
		AddToCellInPlace(grid[DOOR_I][DOOR_J], GameObject::DOOR);
		AddToCellInPlace(grid[12][3], GameObject::KEY);
		AddToCellInPlace(grid[4][12], GameObject::IS_TEXT);

		Coordinate baba1{ 12, 2 };
		Coordinate baba2{ 5, 12 };
		return std::make_shared<GameState>(grid, baba1, baba2);
	}

}  // namespace BabaSolver
