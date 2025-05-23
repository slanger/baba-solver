// Tests for the Solver class.

#include "pch.h"

#include "GameState.h"
#include "Solver.h"

TEST(SolverTest, FindsSolution)
{
	std::shared_ptr<BabaSolver::GameState> initial_state = BabaSolver::TestLevel();
	BabaSolver::SolverOptions options;
	std::shared_ptr<BabaSolver::GameState> end_state = BabaSolver::Solve("Test Level", initial_state, options);
	ASSERT_TRUE(end_state);
	EXPECT_TRUE(end_state->HaveWon());
}
