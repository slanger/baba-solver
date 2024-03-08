// Code for solving Baba Is You levels.

#pragma once

#include <memory>

#include "GameState.h"

namespace BabaSolver
{
	std::shared_ptr<GameState> Solve(const std::shared_ptr<GameState>& initial_state);

}  // namespace BabaSolver
