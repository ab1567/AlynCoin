#ifndef STATE_CIRCUIT_H
#define STATE_CIRCUIT_H

#include <string>
#include <unordered_map>
#include <vector>

class StateCircuit {
public:
    StateCircuit();

    // ✅ Hash commitments
    std::string computeStateRootHash() const;

    void addAccountState(const std::string& address, double balance);

private:
    std::unordered_map<std::string, double> accountStates;
    std::string hashAccountData(const std::string& address, double balance) const;
};

#endif // STATE_CIRCUIT_H
