#ifndef ALYNCOIN_CONSENSUS_REWARD_H
#define ALYNCOIN_CONSENSUS_REWARD_H

namespace consensus {

// Subsidy depends only on circulating supply before the block.
double calculateBlockSubsidy(double supplyBefore);

} // namespace consensus

#endif // ALYNCOIN_CONSENSUS_REWARD_H
