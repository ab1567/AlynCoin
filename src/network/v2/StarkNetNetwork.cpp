#include "StarkNetNetwork.h"
#include "generated/block_protos.pb.h"
#include "generated/sync_protos.pb.h"
#include "block.h"
#include "crypto_utils.h"
#include <mutex>
#include <unordered_map>
#include <array>
#include <cstring>
#include <iostream>

using boost::asio::ip::udp;

struct EpochProofEntry {
    std::string root;
    std::vector<uint8_t> proof;
};
static std::unordered_map<int, EpochProofEntry> receivedEpochProofs;
static std::mutex epochProofMutex;

StarkNetNetwork::StarkNetNetwork() = default;
StarkNetNetwork::~StarkNetNetwork() { stop(); }

void StarkNetNetwork::start() {
    m_socket = std::make_unique<udp::socket>(m_ctx, udp::endpoint(udp::v4(), 0));
    m_running = true;
    m_ioThread = std::thread(&StarkNetNetwork::ioLoop, this);
}

void StarkNetNetwork::stop() {
    m_running = false;
    if (m_socket) {
        boost::system::error_code ec; m_socket->close(ec);
    }
    if (m_ioThread.joinable()) m_ioThread.join();
}

void StarkNetNetwork::connectTo(const std::string& uri) {
    auto pos = uri.find(':');
    if (pos == std::string::npos) {
        std::cerr << "[StarkNet] invalid peer URI: " << uri << "\n";
        return;
    }
    std::string host = uri.substr(0, pos);
    std::string port = uri.substr(pos + 1);
    udp::resolver res(m_ctx);
    boost::system::error_code ec;
    auto result = res.resolve(host, port, ec);
    if (!ec && result.begin() != result.end()) {
        m_peers.push_back(result.begin()->endpoint());
        std::cout << "[starknet] added peer " << uri << "\n";
    } else {
        std::cerr << "[StarkNet] failed to resolve " << uri << "\n";
    }
}

void StarkNetNetwork::broadcastBlock(const Block& blk) {
    if (!m_socket) return;
    alyncoin::BlockProto proto = blk.toProtobuf();
    std::string data;
    proto.SerializeToString(&data);
    std::string frame = "ALYN|BLOCK_BROADCAST|" + data;
    for (const auto& ep : m_peers) {
        boost::system::error_code ec;
        m_socket->send_to(boost::asio::buffer(frame), ep, 0, ec);
    }
    if (m_onBlock) m_onBlock(blk);
}

void StarkNetNetwork::requestBlock(uint64_t idx) {
    if (!m_socket) return;
    alyncoin::BlockRequestProto req;
    req.set_request_type("block_by_height");
    req.set_block_index(static_cast<int32_t>(idx));
    std::string data;
    req.SerializeToString(&data);
    std::string frame = "ALYN|BLOCK_REQUEST|" + data;
    for (const auto& ep : m_peers) {
        boost::system::error_code ec;
        m_socket->send_to(boost::asio::buffer(frame), ep, 0, ec);
    }
}

void StarkNetNetwork::broadcastEpochProof(int epochIdx, const std::string& root,
                                          const std::vector<uint8_t>& proof) {
    if (!m_socket) return;
    std::string b64 = Crypto::base64Encode(std::string(proof.begin(), proof.end()), false);
    std::string frame = "ALYN|EPOCH_PROOF|" + std::to_string(epochIdx) + "|" + root + "|" + b64;
    for (const auto& ep : m_peers) {
        boost::system::error_code ec;
        m_socket->send_to(boost::asio::buffer(frame), ep, 0, ec);
    }
}

void StarkNetNetwork::requestEpochHeaders(const std::string& peerId) {
    if (!m_socket) return;
    (void)peerId; // broadcast to all for now
    std::string frame = "ALYN|REQUEST_EPOCH_HEADERS";
    for (const auto& ep : m_peers) {
        boost::system::error_code ec;
        m_socket->send_to(boost::asio::buffer(frame), ep, 0, ec);
    }
}

void StarkNetNetwork::ioLoop() {
    std::array<char, 8192> buf{};
    udp::endpoint sender;
    while (m_running) {
        boost::system::error_code ec;
        size_t n = m_socket->receive_from(boost::asio::buffer(buf), sender, 0, ec);
        if (ec) {
            if (ec == boost::asio::error::would_block) continue;
            continue;
        }
        std::string msg(buf.data(), n);
        handleDatagram(msg, sender);
    }
}

void StarkNetNetwork::handleDatagram(const std::string& msg, const udp::endpoint& from) {
    if (msg.rfind("ALYN|BLOCK_BROADCAST|", 0) == 0) {
        std::string body = msg.substr(strlen("ALYN|BLOCK_BROADCAST|"));
        alyncoin::BlockProto pb;
        if (pb.ParseFromString(body)) {
            try {
                Block blk = Block::fromProto(pb, true);
                if (m_onBlock) m_onBlock(blk);
            } catch (const std::exception& e) {
                std::cerr << "[starknet] bad block from " << from.address() << "\n";
            }
        }
    } else if (msg.rfind("ALYN|BLOCK_REQUEST|", 0) == 0) {
        std::string body = msg.substr(strlen("ALYN|BLOCK_REQUEST|"));
        alyncoin::BlockRequestProto req;
        if (req.ParseFromString(body)) {
            std::cout << "[starknet] block request for " << req.block_index() << "\n";
        }
    } else if (msg.rfind("ALYN|EPOCH_PROOF|", 0) == 0) {
        std::string body = msg.substr(strlen("ALYN|EPOCH_PROOF|"));
        size_t p1 = body.find('|');
        size_t p2 = body.find('|', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) return;
        int epoch = std::stoi(body.substr(0, p1));
        std::string root = body.substr(p1 + 1, p2 - p1 - 1);
        std::string proofB64 = body.substr(p2 + 1);
        std::string raw = Crypto::base64Decode(proofB64, false);
        std::vector<uint8_t> proof(raw.begin(), raw.end());
        std::lock_guard<std::mutex> lk(epochProofMutex);
        receivedEpochProofs[epoch] = {root, proof};
        std::cout << "[starknet] epoch proof " << epoch << " received\n";
    } else if (msg == "ALYN|REQUEST_EPOCH_HEADERS") {
        std::cout << "[starknet] epoch header request from " << from.address() << "\n";
    }
}
