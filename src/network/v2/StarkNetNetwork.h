#ifndef STARKNET_NETWORK_H
#define STARKNET_NETWORK_H

#include "INetwork.h"
#include <thread>
#include <atomic>

/**
 * Experimental transport focused on STARK-compressed proofs and epoch sync.
 *  - QUIC transport
 *  - snappy + BLAKE3 framing
 *  - BLOCK_WRAPPER messages
 */
class StarkNetNetwork final : public INetwork {
public:
    explicit StarkNetNetwork(unsigned short port = 15671);
    ~StarkNetNetwork() override;

    void start() override;
    void stop() override;

    void connectTo(const std::string& uri) override;
    void broadcastBlock(const Block& blk) override;
    void requestBlock(uint64_t index) override;

    void onBlock(BlockHandler h) override { m_onBlock = std::move(h); }
private:
    void ioLoop();

    BlockHandler      m_onBlock;
    std::thread       m_ioThread;
    std::atomic<bool> m_running{false};
    unsigned short    m_port;
};

#endif // STARKNET_NETWORK_H
