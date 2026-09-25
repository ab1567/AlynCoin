#include "state_circuit.h"
#include "../rollup_utils.h"
#include <sstream>

StateCircuit::StateCircuit() {}

std::string StateCircuit::computeStateRootHash() const {
    std::vector<std::string> accountHashes;
    for (const auto& [address, balance] : accountStates) {
        accountHashes.push_back(hashAccountData(address, balance));
    }

    return RollupUtils::calculateMerkleRoot(accountHashes);
}

std::string StateCircuit::hashAccountData(const std::string& address, double balance) const {
    std::ostringstream ss;
    ss << address << balance;
    return RollupUtils::hybridHashWithDomain(ss.str(), "StateTrace");
}
void StateCircuit::addAccountState(const std::string& address, double balance) {
    accountStates[address] = balance;
}
