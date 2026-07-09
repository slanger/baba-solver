#include "Replayer.h"

#include <chrono>
#include <memory>
#include <print>
#include <string_view>
#include <thread>

#include "GameState.h"
#include "Solver.h"

namespace BabaSolver {

static constexpr std::chrono::milliseconds TIME_BETWEEN_FRAMES(350);
// ANSI escape code to clear the screen in the terminal.
static constexpr std::string_view CLEAR_SCREEN = "\033[2J";
// ANSI escape code to move the cursor to home position (0, 0).
static constexpr std::string_view MOVE_TO_HOME_POSITION = "\033[H";

void ReplayResult(const SolverResult& result) {
  std::println("Replaying result...");
  std::print(CLEAR_SCREEN);

  for (const IterationResult& iter : result.iterations) {
    std::shared_ptr<GameState> current_state = iter.initial_state;
    std::print(MOVE_TO_HOME_POSITION);
    current_state->PrintGrid();
    std::this_thread::sleep_for(TIME_BETWEEN_FRAMES);
    for (Direction direction : iter.end_state->Moves()) {
      current_state = current_state->ApplyMove(direction);
      std::print(MOVE_TO_HOME_POSITION);
      current_state->PrintGrid();
      std::this_thread::sleep_for(TIME_BETWEEN_FRAMES);
    }
  }
}

}  // namespace BabaSolver
