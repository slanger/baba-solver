// Tests for the Solver class.

#include "pch.h"

#include "GameState.h"
#include "Solver.h"

TEST(SolverTest, FindsSolution)
{
	std::shared_ptr<BabaSolver::GameState> initial_state = BabaSolver::TestLevel();
	std::shared_ptr<BabaSolver::GameState> winning_state = BabaSolver::Solve(initial_state);
	ASSERT_TRUE(winning_state);
}
