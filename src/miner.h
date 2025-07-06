#ifndef MINER_H
#define MINER_H

#include "blockchain.h"
#include <atomic>
#include <string>

extern std::atomic<bool> miningActive;
extern std::atomic<bool> miningPaused;

class Miner {
public:
  static std::string mineBlock(int difficulty); // ✅ FIXED: Add inside class
  static void startMiningProcess(const std::string &minerAddress);
  static void pauseMining();
  static void resumeMining();
};

#endif // MINER_H
