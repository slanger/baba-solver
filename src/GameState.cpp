#include "GameState.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace BabaSolver {

std::size_t GameStatePtrHasher::operator()(
    const std::shared_ptr<GameState>& state) const {
  return state->Hash();
}

bool GameStatePtrComparer::operator()(
    const std::shared_ptr<GameState>& lhs,
    const std::shared_ptr<GameState>& rhs) const {
  return lhs->Equals(*rhs);
}

}  // namespace BabaSolver
