#include "INetwork.h"
#include "LegacyNetwork.h"
#include "StarkNetNetwork.h"
#include <algorithm>
#include <stdexcept>

std::unique_ptr<INetwork> make_network(const std::string& impl,
                                       unsigned short port,
                                       Blockchain* chain,
                                       PeerBlacklist* blacklist) {
    std::string key = impl;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (key == "legacy")
        return std::make_unique<LegacyNetwork>(port, chain, blacklist);
    if (key == "stark")
        return std::make_unique<StarkNetNetwork>(port);
    throw std::invalid_argument("unknown network impl: " + impl);
}
