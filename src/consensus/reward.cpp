#include "consensus/reward.h"

#include "block_reward.h"
#include "constants.h"

#include <algorithm>

namespace consensus {
namespace {
// Reward decays smoothly with the circulating supply. By tying the emission to
// totalSupply/MAX_SUPPLY we guarantee a monotonic decrease as supply grows.
inline double supplyDecayReward(double supplyBefore) {
  if (MAX_SUPPLY <= 0.0)
    return 0.0;

  const double ratio = std::clamp(supplyBefore / MAX_SUPPLY, 0.0, 1.0);
  const double reward = INITIAL_REWARD * (1.0 - ratio);
  return std::max(0.0, reward);
}

} // namespace

double calculateBlockSubsidy(double supplyBefore) {
  const double remaining = std::max(0.0, MAX_SUPPLY - supplyBefore);
  if (remaining <= 0.0)
    return 0.0;

  double reward = supplyDecayReward(supplyBefore);

  return std::min(reward, remaining);
}

} // namespace consensus
