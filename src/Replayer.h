// Code for replaying the result of a BabaSolver run.

#pragma once

#include "Solver.h"

namespace BabaSolver {

// Replays the result of a BabaSolver run by animating the game in stdout.
void ReplayResult(const SolverResult& result);

}  // namespace BabaSolver
