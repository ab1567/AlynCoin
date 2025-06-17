#include "LegacyNetwork.h"
#include "../network.h"

#include <cstdlib>
#include <iostream>

LegacyNetwork::LegacyNetwork(unsigned short port,
                             Blockchain* chain,
                             PeerBlacklist* blacklist)
    : m_port(port), m_chain(chain), m_blacklist(blacklist), m_native(nullptr) {}

LegacyNetwork::~LegacyNetwork() = default;

void LegacyNetwork::start() {
    if (!m_native) {
        try {
            m_native = &Network::getInstance(m_port, m_chain, m_blacklist);
        } catch (const std::exception& e) {
            std::cerr << "[LegacyNetwork] init failed: " << e.what() << "\n";
            return;
        }
    }
    m_native->start();
}

void LegacyNetwork::stop() {
    // the legacy network cleans up in its destructor; nothing explicit here
}

void LegacyNetwork::connectTo(const std::string& uri) {
    if (!m_native) return;
    std::string host = uri;
    int port = m_port;
    auto pos = uri.find(':');
    if (pos != std::string::npos) {
        host = uri.substr(0, pos);
        port = std::atoi(uri.substr(pos + 1).c_str());
    }
    m_native->connectToPeer(host, static_cast<short>(port));
}

void LegacyNetwork::broadcastBlock(const Block& blk) {
    if (m_native) {
        m_native->broadcastBlock(blk);
    }
    if (m_onBlock) m_onBlock(blk);
}

void LegacyNetwork::requestBlock(uint64_t /*index*/) {
    if (!m_native) return;
    // legacy path only provides full chain sync for now
    // choose an arbitrary peer if available
    auto peers = m_native->getPeerManager()->getConnectedPeers();
    if (!peers.empty()) {
        m_native->requestBlockchainSync(peers.front());
    }
}
