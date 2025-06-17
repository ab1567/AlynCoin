#ifndef STARKNET_NETWORK_H
#define STARKNET_NETWORK_H

#include "INetwork.h"
#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <vector>

/**
 * Experimental transport focused on STARK-compressed proofs and epoch sync.
 *  - QUIC transport
 *  - snappy + BLAKE3 framing
 *  - BLOCK_WRAPPER messages
 */
class StarkNetNetwork final : public INetwork {
public:
    StarkNetNetwork();
    ~StarkNetNetwork() override;

    void start() override;
    void stop() override;

    void connectTo(const std::string& uri) override;
    void broadcastBlock(const Block& blk) override;
    void requestBlock(uint64_t index) override;

    void onBlock(BlockHandler h) override { m_onBlock = std::move(h); }
private:
    void ioLoop();
    void handleDatagram(const std::string& msg,
                        const boost::asio::ip::udp::endpoint& from);

    BlockHandler      m_onBlock;
    std::thread       m_ioThread;
    std::atomic<bool> m_running{false};

    boost::asio::io_context                   m_ctx;
    std::unique_ptr<boost::asio::ip::udp::socket> m_socket;
    std::vector<boost::asio::ip::udp::endpoint>   m_peers;
};

#endif // STARKNET_NETWORK_H
