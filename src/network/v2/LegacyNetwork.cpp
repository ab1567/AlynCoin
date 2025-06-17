#include "LegacyNetwork.h"
#include "../network.h"           // existing implementation
#include "generated/sync_protos.pb.h"
#include <iostream>

LegacyNetwork::LegacyNetwork() = default;
LegacyNetwork::~LegacyNetwork() = default;

void LegacyNetwork::start() {
    if (auto n = legacy()) {
        n->start();
    }
}

void LegacyNetwork::stop() {
    // Legacy Network singleton has no explicit stop API
}

void LegacyNetwork::connectTo(const std::string& uri) {
    if (auto n = legacy()) {
        auto pos = uri.find(':');
        if (pos == std::string::npos) {
            std::cerr << "[LegacyNetwork] invalid peer URI: " << uri << "\n";
            return;
        }
        std::string host = uri.substr(0, pos);
        short port = static_cast<short>(std::stoi(uri.substr(pos + 1)));
        n->connectToPeer(host, port);
    }
}

void LegacyNetwork::broadcastBlock(const Block& blk) {
    if (auto n = legacy()) {
        n->broadcastBlock(blk);
    }
    if (m_onBlock) m_onBlock(blk);
}

void LegacyNetwork::requestBlock(uint64_t index) {
    if (auto n = legacy()) {
        alyncoin::BlockRequestProto req;
        req.set_request_type("block_by_height");
        req.set_block_index(static_cast<int32_t>(index));
        std::string data;
        req.SerializeToString(&data);
        n->broadcastMessage("ALYN|BLOCK_REQUEST|" + data);
    }
}

Network* LegacyNetwork::legacy() {
    return Network::getExistingInstance();
}
