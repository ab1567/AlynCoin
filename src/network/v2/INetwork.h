#ifndef INETWORK_H
#define INETWORK_H

#include <memory>
#include <string>
#include <vector>
#include <functional>

struct Block;            // forward declarations from existing types
struct PeerInfo;         // address, ports, keys...

/**
 * Abstract transport / gossip interface.
 * Implementations: LegacyNetwork, StarkNetNetwork.
 */
class INetwork {
public:
    virtual ~INetwork() = default;

    // ---- connection lifecycle --------------------------------------------
    virtual void start() = 0;                 // spin up threads & bind sockets
    virtual void stop()  = 0;

    // ---- gossip -----------------------------------------------------------
    virtual void connectTo(const std::string& uri) = 0;          // add peer (+ dial)
    virtual void broadcastBlock(const Block& blk)    = 0;        // push new block
    virtual void requestBlock(uint64_t index)        = 0;        // pull by height

    // ---- callbacks --------------------------------------------------------
    using BlockHandler = std::function<void(const Block&)>;
    virtual void onBlock(BlockHandler h) = 0;                    // subscribe
};

struct Blockchain;
class PeerBlacklist;

std::unique_ptr<INetwork> make_network(const std::string& impl_name,
                                       unsigned short port,
                                       Blockchain* chain,
                                       PeerBlacklist* blacklist);

#endif // INETWORK_H
