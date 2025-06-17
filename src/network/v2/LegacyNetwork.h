#ifndef LEGACY_NETWORK_H
#define LEGACY_NETWORK_H

#include "INetwork.h"

/** Thin wrapper around existing Network class in network.cpp */
class LegacyNetwork final : public INetwork {
public:
    LegacyNetwork();
    ~LegacyNetwork() override;

    void start() override;
    void stop() override;

    void connectTo(const std::string& uri) override;
    void broadcastBlock(const Block& blk) override;
    void requestBlock(uint64_t index) override;

    void onBlock(BlockHandler h) override { m_onBlock = std::move(h); }
private:
    BlockHandler m_onBlock;
    // pointer or handle to the old singleton will be added later
};

#endif // LEGACY_NETWORK_H
