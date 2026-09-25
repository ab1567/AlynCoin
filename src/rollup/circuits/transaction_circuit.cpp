#include "transaction_circuit.h"
#include "../rollup_utils.h"
#include <sstream>

TransactionCircuit::TransactionCircuit() {}

void TransactionCircuit::addTransactionData(const std::string& sender, const std::string& recipient, double amount, const std::string& txHash) {
    std::ostringstream ss;
    ss << sender << recipient << amount << txHash;
    std::string hashed = RollupUtils::hybridHashWithDomain(ss.str(), "TxTrace");

    {
        std::lock_guard<std::mutex> lock(traceMutex);
        transactionTrace.push_back(hashed);
    }

    computeMerkleRoot();
}

void TransactionCircuit::computeMerkleRoot() {
    merkleRoot = RollupUtils::calculateMerkleRoot(transactionTrace);
}

std::string TransactionCircuit::getMerkleRoot() const {
    return merkleRoot;
}
