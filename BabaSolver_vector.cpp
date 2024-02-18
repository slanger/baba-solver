// BabaSolver solves the level "The Floatiest Platforms" in the game Baba Is You.
//
// The algorithm is a brute force algorithm, calculating all the moves you can do and printing out
// the first one that succeeds in beating the level.

// TODO:
// * Implement "rock is push" rule, including breaking the rule. DONE
// * Allow Baba to die if it falls off the platform. DONE
// * Implement win condition. (Shortcut: Key touching door wins.) DONE

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <stack>
#include <vector>

struct Coordinate
{
	short i;
	short j;
};

enum class Direction : short
{
	UP,
	RIGHT,
	DOWN,
	LEFT,
};

enum class GameObject : short
{
	IMMOVABLE,
	TILE,
	BABA,
	ROCK,
	DOOR,
	KEY,
	ROCK_TEXT,
	IS_TEXT,
	PUSH_TEXT,
};

const short MAX_TURNS = 10;
const short GRID_HEIGHT = 20;
const short GRID_WIDTH = 20;
const short BABA_DEAD = -1;
const short DOOR_I = 13;
const short DOOR_J = 5;
const short IS_TEXT_START_I = 5;
const short IS_TEXT_START_J = 13;

class GameState
{
public:
	long long _turn;
	std::vector<GameObject> _grid[GRID_HEIGHT][GRID_WIDTH];
	bool _won;

private:
	Coordinate _baba1;
	Coordinate _baba2;
	Coordinate _is_text;
	bool _rock_is_push_active;

public:
	GameState(long long turn, Coordinate baba1, Coordinate baba2, Coordinate is_text, bool rock_is_push_active)
		: _turn(turn), _baba1(baba1), _baba2(baba2), _is_text(is_text), _rock_is_push_active(rock_is_push_active), _won(false) {}

	std::shared_ptr<GameState> ApplyMove(Direction direction)
	{
		std::shared_ptr<GameState> new_state = std::make_shared<GameState>(_turn+1, _baba1, _baba2, _is_text, _rock_is_push_active);
		std::copy(&_grid[0][0], &_grid[0][0] + (GRID_HEIGHT * GRID_WIDTH), &new_state->_grid[0][0]);
		new_state->MoveBaba(new_state->_baba1, direction);
		new_state->MoveBaba(new_state->_baba2, direction);
		new_state->RecalculateState();
		return new_state;
	}

	bool AllBabasDead()
	{
		return _baba1.i == BABA_DEAD && _baba2.i == BABA_DEAD;
	}

	bool BabasOnSameSpace()
	{
		if (_baba1.i == BABA_DEAD || _baba2.i == BABA_DEAD)
			return false;
		return _baba1.i == _baba2.i && _baba1.j == _baba2.j;
	}

	void Print()
	{
		for (short i = 0; i < GRID_HEIGHT; ++i)
		{
			for (int j = 0; j < GRID_WIDTH; ++j)
			{
				if (_grid[i][j].empty())
				{
					std::cout << ' ';
					continue;
				}
				if ((i == _baba1.i && j == _baba1.j) || (i == _baba2.i && j == _baba2.j))
				{
					std::cout << 'B';
					continue;
				}

				char c;
				switch (_grid[i][j].back())
				{
				case GameObject::IMMOVABLE: c = 'X'; break;
				case GameObject::TILE: c = '^'; break;
				case GameObject::BABA: c = 'B'; break;
				case GameObject::ROCK: c = 'R'; break;
				case GameObject::DOOR: c = 'D'; break;
				case GameObject::KEY: c = 'K'; break;
				case GameObject::ROCK_TEXT: c = '1'; break;
				case GameObject::IS_TEXT: c = '2'; break;
				case GameObject::PUSH_TEXT: c = '3'; break;
				default: throw "Invalid GameObject";
				}
				std::cout << c;
			}
			std::cout << '\n';
		}
		std::cout << std::endl;
	}

private:
	void MoveBaba(Coordinate& baba, Direction direction)
	{
		if (baba.i == BABA_DEAD)
			return;

		int deltaI = 0;
		int deltaJ = 0;
		switch (direction)
		{
		case Direction::UP:
			deltaI = -1;
			break;
		case Direction::RIGHT:
			deltaJ = 1;
			break;
		case Direction::DOWN:
			deltaI = 1;
			break;
		case Direction::LEFT:
			deltaJ = -1;
			break;
		default:
			throw "Invalid direction";
		}

		bool canMove = CheckCellAndMove(baba.i + deltaI, baba.j + deltaJ, direction);
		if (!canMove)
			return;
		baba.i += deltaI;
		baba.j += deltaJ;
	}

	bool CheckCellAndMove(int i, int j, Direction direction)
	{
		if (i < 0 || i >= GRID_HEIGHT || j < 0 || j >= GRID_WIDTH)
			throw "CheckCellAndMove: We shouldn't be able to reach here.";

		bool objToMove = false;
		for (GameObject obj : _grid[i][j])
		{
			if (obj == GameObject::IMMOVABLE || obj == GameObject::DOOR)
			{
				return false;
			}
			else if (obj == GameObject::ROCK || obj == GameObject::KEY || obj == GameObject::ROCK_TEXT
				|| obj == GameObject::IS_TEXT || obj == GameObject::PUSH_TEXT)
			{
				objToMove = true;
			}
		}
		if (!objToMove)
			return true;

		int deltaI = 0;
		int deltaJ = 0;
		switch (direction)
		{
		case Direction::UP:
			deltaI = -1;
			break;
		case Direction::RIGHT:
			deltaJ = 1;
			break;
		case Direction::DOWN:
			deltaI = 1;
			break;
		case Direction::LEFT:
			deltaJ = -1;
			break;
		default:
			throw "Invalid direction";
		}
		bool canMove = CheckCellAndMove(i + deltaI, j + deltaJ, direction);
		if (!canMove)
			return false;

		auto iter = _grid[i][j].begin();
		while (iter != _grid[i][j].end())
		{
			GameObject obj = *iter;
			if (obj == GameObject::ROCK || obj == GameObject::KEY || obj == GameObject::ROCK_TEXT
				|| obj == GameObject::IS_TEXT || obj == GameObject::PUSH_TEXT)
			{
				// Actually move the game object in the grid.
				iter = _grid[i][j].erase(iter);
				_grid[i + deltaI][j + deltaJ].push_back(obj);
				if (obj == GameObject::IS_TEXT)
				{
					_is_text.i = i + deltaI;
					_is_text.j = j + deltaJ;
				}
			}
			else
			{
				++iter;
			}
		}
		return true;
	}

	void RecalculateState()
	{
		// Check if we won.
		// Optimization: The Door's coordinates are hard-coded because it doesn't move.
		const std::vector<GameObject>& door_cell = _grid[DOOR_I][DOOR_J];
		if (std::find(door_cell.cbegin(), door_cell.cend(), GameObject::KEY) != door_cell.cend())
		{
			_won = true;
		}

		// Check if any of the Babas are dead.
		if (_baba1.i != _baba2.i || _baba1.j != _baba2.j)
		{
			if (_baba1.i != BABA_DEAD && _grid[_baba1.i][_baba1.j].empty())
			{
				_baba1.i = BABA_DEAD;
				_baba1.j = BABA_DEAD;
			}
			if (_baba2.i != BABA_DEAD && _grid[_baba2.i][_baba2.j].empty())
			{
				_baba2.i = BABA_DEAD;
				_baba2.j = BABA_DEAD;
			}
		}

		// Check if the "rock is push" rule is active.
		// TODO: We can store the coordinates of the IS text to make this faster.
		_rock_is_push_active = CheckRockIsPushIntact();
	}

	bool CheckRockIsPushIntact()
	{
		if (_is_text.i > 0 && _is_text.i < GRID_HEIGHT - 1)
		{
			// Check "rock is push" rule vertically.
			const std::vector<GameObject>& up_one = _grid[_is_text.i - 1][_is_text.j];
			const std::vector<GameObject>& down_one = _grid[_is_text.i + 1][_is_text.j];
			if (std::find(up_one.cbegin(), up_one.cend(), GameObject::ROCK_TEXT) != up_one.cend() &&
				std::find(down_one.cbegin(), down_one.cend(), GameObject::PUSH_TEXT) != down_one.cend())
			{
				return true;
			}
		}
		if (_is_text.j > 0 && _is_text.j < GRID_WIDTH - 1)
		{
			// Check "rock is push" rule horizontally.
			const std::vector<GameObject>& left_one = _grid[_is_text.i][_is_text.j - 1];
			const std::vector<GameObject>& right_one = _grid[_is_text.i][_is_text.j + 1];
			if (std::find(left_one.cbegin(), left_one.cend(), GameObject::ROCK_TEXT) != left_one.cend() &&
				std::find(right_one.cbegin(), right_one.cend(), GameObject::PUSH_TEXT) != right_one.cend())
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

int main()
{
	std::cout << "Baba Is You solver" << std::endl;
	std::shared_ptr<GameState> initial_state = std::make_shared<GameState>(
		0, Coordinate{ 6, 5 }, Coordinate{ 6, 13 }, Coordinate{ IS_TEXT_START_I, IS_TEXT_START_J }, true);
	// Add a boundary of immovables around the play area.
	for (short j = 0; j < GRID_WIDTH; ++j)
	{
		initial_state->_grid[0][j].push_back(GameObject::IMMOVABLE);
		initial_state->_grid[GRID_HEIGHT-1][j].push_back(GameObject::IMMOVABLE);
	}
	for (short i = 1; i < GRID_HEIGHT-1; ++i)
	{
		initial_state->_grid[i][0].push_back(GameObject::IMMOVABLE);
		initial_state->_grid[i][GRID_WIDTH-1].push_back(GameObject::IMMOVABLE);
	}
	// Add tiles to grid.
	for (short i = 4; i <= 8; ++i)
	{
		for (short j = 3; j <= 7; ++j)
		{
			initial_state->_grid[i][j].push_back(GameObject::TILE);
		}
	}
	for (short i = 4; i <= 8; ++i)
	{
		for (short j = 11; j <= 15; ++j)
		{
			initial_state->_grid[i][j].push_back(GameObject::TILE);
		}
	}
	for (short i = 11; i <= 15; ++i)
	{
		for (short j = 3; j <= 7; ++j)
		{
			initial_state->_grid[i][j].push_back(GameObject::TILE);
		}
	}
	for (short i = 10; i <= 14; ++i)
	{
		for (short j = 11; j <= 15; ++j)
		{
			initial_state->_grid[i][j].push_back(GameObject::TILE);
		}
	}
	// Add an immovable for each text block around the corners of the grid.
	initial_state->_grid[1][1].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[1][2].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[1][3].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[1][8].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[1][9].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[1][10].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[17][1].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[17][2].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][1].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][2].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][3].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][4].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[16][16].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[16][17].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[16][18].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[17][16].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[17][17].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[17][18].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][16].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][17].push_back(GameObject::IMMOVABLE);
	initial_state->_grid[18][18].push_back(GameObject::IMMOVABLE);
	// Add the other game objects to the grid.
	initial_state->_grid[5][4].push_back(GameObject::ROCK);
	initial_state->_grid[7][6].push_back(GameObject::ROCK);
	initial_state->_grid[7][12].push_back(GameObject::ROCK);
	initial_state->_grid[5][12].push_back(GameObject::ROCK_TEXT);
	initial_state->_grid[IS_TEXT_START_I][IS_TEXT_START_J].push_back(GameObject::IS_TEXT);
	initial_state->_grid[5][14].push_back(GameObject::PUSH_TEXT);
	initial_state->_grid[17][3].push_back(GameObject::PUSH_TEXT);
	initial_state->_grid[DOOR_I][DOOR_J].push_back(GameObject::DOOR);
	initial_state->_grid[12][13].push_back(GameObject::KEY);

	initial_state->Print();

	std::stack<NextMove> stack;
	// Apply initial four directions to the stack.
	stack.push(NextMove{ initial_state, Direction::UP });
	stack.push(NextMove{ initial_state, Direction::RIGHT });
	stack.push(NextMove{ initial_state, Direction::DOWN });
	stack.push(NextMove{ initial_state, Direction::LEFT });
	initial_state.reset();

	long long num_moves = 0;
	bool won = false;
	bool found_invincible_babas_trick = false;
	auto start_time = std::chrono::high_resolution_clock::now();
	while (!stack.empty())
	{
		++num_moves;
		if (num_moves % 10000 == 0)
			std::cout << "Calculating move #" << num_moves << ", stack size = " << stack.size() << std::endl;
		const NextMove& cur = stack.top();
		std::shared_ptr<GameState> new_state = cur.state->ApplyMove(cur.dir_to_apply);
		stack.pop();
		if (new_state->_won)
		{
			won = true;
			std::cout << "WIN!!!\n";
			new_state->Print();
			break;
		}
		if (!found_invincible_babas_trick && new_state->BabasOnSameSpace())
		{
			found_invincible_babas_trick = true;
			std::cout << "Found invincible Babas trick!!! Move #" << num_moves << ", turn #" << new_state->_turn << std::endl;
			new_state->Print();
		}
		if (new_state->AllBabasDead() || new_state->_turn >= MAX_TURNS)
		{
			// new_state->Print();
			continue;
		}
		stack.push(NextMove{ new_state, Direction::UP });
		stack.push(NextMove{ new_state, Direction::RIGHT });
		stack.push(NextMove{ new_state, Direction::DOWN });
		stack.push(NextMove{ new_state, Direction::LEFT });
		new_state.reset();
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

	if (!won)
	{
		std::cout << "Did not win...\n";
	}
	if (found_invincible_babas_trick)
	{
		std::cout << "Found invincible Babas trick!!!\n";
	}
	else
	{
		std::cout << "Did not find the invincible Babas trick...\n";
	}
	std::cout << "Max move depth: " << MAX_TURNS << "\n";
	std::cout << "Number of moves simulated: " << num_moves << "\n";
	std::cout << "Total time: " << total_time.count() << " microseconds\n";
	std::cout << "Time per move: " << (total_time.count() / num_moves) << " microseconds" << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
