#include "LegacyNetwork.h"
#include "../network.h" // existing implementation

LegacyNetwork::LegacyNetwork() = default;
LegacyNetwork::~LegacyNetwork() = default;

void LegacyNetwork::start() {
    // TODO: delegate to existing Network singleton
}

void LegacyNetwork::stop() {
    // TODO: delegate
}

void LegacyNetwork::connectTo(const std::string& uri) {
    // TODO: delegate
}

void LegacyNetwork::broadcastBlock(const Block& blk) {
    // TODO: delegate
    if (m_onBlock) m_onBlock(blk);
}

void LegacyNetwork::requestBlock(uint64_t index) {
    // TODO: delegate
}
