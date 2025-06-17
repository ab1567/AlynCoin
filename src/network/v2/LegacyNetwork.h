#ifndef LEGACY_NETWORK_H
#define LEGACY_NETWORK_H

#include "INetwork.h"
#include "../network.h"
#include "blockchain.h"
#include "network/peer_blacklist.h"

/** Thin wrapper around existing Network class in network.cpp */
class LegacyNetwork final : public INetwork {
public:
    LegacyNetwork(unsigned short port,
                  Blockchain* chain,
                  PeerBlacklist* blacklist);
    ~LegacyNetwork() override;

    void start() override;
    void stop() override;

    void connectTo(const std::string& uri) override;
    void broadcastBlock(const Block& blk) override;
    void requestBlock(uint64_t index) override;

    void onBlock(BlockHandler h) override { m_onBlock = std::move(h); }
private:
    BlockHandler m_onBlock;
    unsigned short m_port;
    Blockchain* m_chain;
    PeerBlacklist* m_blacklist;
    Network* m_native; // underlying legacy implementation
};

#endif // LEGACY_NETWORK_H
