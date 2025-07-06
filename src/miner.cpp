#include "generated/block_protos.pb.h"
#include "generated/sync_protos.pb.h"
#include "miner.h"
#include "blake3.h"
#include "blockchain.h"
#include "crypto_utils.h"
#include "mining.h"
#include <atomic>
#include "network.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

std::atomic<bool> miningActive{false};
std::atomic<bool> miningPaused{false};

bool containsValidTransaction(const std::vector<Transaction> &transactions) {
    for (const auto &tx : transactions) {
        if (tx.getSender() != "System") {
            return true;
        }
    }
    return false;
}

void Miner::startMiningProcess(const std::string &minerAddress) {
    try {
        std::cout << "🚀 Starting mining process for: " << minerAddress << std::endl;

        if (miningActive.exchange(true)) {
            std::cerr << "⚠️ Mining already in progress.\n";
            return;
        }

        Blockchain &blockchain = Blockchain::getInstance();

        blockchain.loadPendingTransactionsFromDB();
        blockchain.reloadBlockchainState();  // Load once before loop

        while (miningActive) {
            if (!Network::isUninitialized()) {
                PeerManager* pm = Network::getInstance().getPeerManager();
                if (pm && blockchain.getHeight() < static_cast<int>(pm->getMedianNetworkHeight()) - 3) {
                    Miner::pauseMining();
                } else {
                    Miner::resumeMining();
                }
            }

            if (miningPaused) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            Block minedBlock = blockchain.mineBlock(minerAddress);

            if (minedBlock.getHash().empty()) {
                std::cerr << "❌ Mining failed. No valid hash generated.\n";
                miningActive = false;
                break;
            }

            const auto &zk = minedBlock.getZkProof();
            const auto &dilKey = minedBlock.getPublicKeyDilithium();
            const auto &falKey = minedBlock.getPublicKeyFalcon();

            if (zk.empty() || dilKey.empty() || falKey.empty()) {
                std::cerr << "⚠️ Invalid block content. Missing zkProof or public keys.\n";
                miningActive = false;
                break;
            }

            if (zk.size() > 4096 || dilKey.size() > 4096 || falKey.size() > 4096) {
                std::cerr << "⚠️ Abnormal field size detected. Aborting block.\n";
                miningActive = false;
                break;
            }

            std::string blockMsg = minedBlock.getHash() + minedBlock.getPreviousHash() +
                                   minedBlock.getTransactionsHash() + std::to_string(minedBlock.getTimestamp());

            std::cout << "[SIGN DEBUG] 🔏 Block Message (MINING): " << blockMsg << std::endl;
            std::cout << "[SIGN DEBUG] 🧬 Dilithium PubKey (MINING): " << Crypto::toHex(dilKey) << std::endl;
            std::cout << "[SIGN DEBUG] 🧬 Falcon PubKey (MINING): " << Crypto::toHex(falKey) << std::endl;

            // ✅ Only add and broadcast if block is valid
            if (blockchain.addBlock(minedBlock)) {
                blockchain.saveToDB();

                if (!Network::isUninitialized()) {
                    Network::getInstance().broadcastBlock(minedBlock);
                }

                std::cout << "✅ Block mined, added and broadcasted. Hash: "
                          << minedBlock.getHash() << "\n";
            } else {
                std::cerr << "❌ addBlock() failed — block not added or broadcasted.\n";
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception &e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
    }
}

// ✅ Improved Mining Algorithm: Hybrid PoW (BLAKE3 + Keccak256)
std::string Miner::mineBlock(int difficulty) {
     std::string lastHash = Blockchain::getInstance().getLatestBlock().getHash();
    int nonce = 0;
    std::string newHash;

    while (true) {
        std::ostringstream ss;
        ss << lastHash << nonce;
        std::string candidateHash = Crypto::hybridHash(ss.str());

        // PoW check: leading `difficulty` zeroes
        if (candidateHash.substr(0, difficulty) == std::string(difficulty, '0')) {
            newHash = candidateHash;
            break;
        }
        nonce++;
    }

    std::cout << "✅ Found valid PoW hash: " << newHash << "\n";
    return newHash;
}

void Miner::pauseMining() { miningPaused = true; }

void Miner::resumeMining() { miningPaused = false; }
