#include "StarkNetNetwork.h"
#include <iostream>

StarkNetNetwork::StarkNetNetwork(unsigned short port)
    : m_port(port) {}
StarkNetNetwork::~StarkNetNetwork() { stop(); }

void StarkNetNetwork::start() {
    m_running = true;
    m_ioThread = std::thread(&StarkNetNetwork::ioLoop, this);
}

void StarkNetNetwork::stop() {
    m_running = false;
    if (m_ioThread.joinable()) m_ioThread.join();
}

void StarkNetNetwork::connectTo(const std::string& uri) {
    std::cout << "[starknet] dialing " << uri << "\n";
    // TODO: QUIC connect
}

void StarkNetNetwork::broadcastBlock(const Block& blk) {
    // TODO: serialize header + recursive proof, compress and send
    (void)blk;
}

void StarkNetNetwork::requestBlock(uint64_t idx) {
    // TODO: send BLOCK_REQUEST message
    (void)idx;
}

void StarkNetNetwork::ioLoop() {
    while (m_running) {
        // TODO: poll sockets and dispatch frames
    }
}
