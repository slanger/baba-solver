// BabaSolver solves the level "The Floatiest Platforms" in the game Baba Is You.
//
// The algorithm is a brute force algorithm, calculating all the moves you can do and printing out
// the first one that succeeds in beating the level. Various optimizations and heuristics prune the
// tree of possible game states for better performance.
//
// For best performance:
// * Run the program in "release" mode, not "debug" mode.
// * Tune the parameters in Parameters.h.

// TODO:
// * Implement "rock is push" rule, including breaking the rule. DONE
// * Allow Baba to die if it falls off the platform. DONE
// * Implement win condition. (Shortcut: Key touching door wins.) DONE
// * Add heuristic for preventing back-and-forth moves. OBSOLETE (see next item)
// * Prune sub-trees based on if we've already calculated them. DONE
// * Get rid of outer perimeter of immovable objects. DONE
// * Parallelize the algorithm. DONE

#include <iostream>
#include <memory>

#include "GameState.h"
#include "Solver.h"

int main()
{
	std::cout << "Baba Is You solver" << std::endl;
	std::shared_ptr<BabaSolver::GameState> initial_state = BabaSolver::FloatiestPlatformsLevel();
	BabaSolver::Solve(initial_state);
}
