#pragma once

#include "../network.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace alyn::cli {

inline std::mutex outputMutex;

enum class ConnectOutcome { Success, Failure, Pending };

inline ConnectOutcome connectPeerWithFeedback(
    Network &network, const std::string &ip, int port,
    std::chrono::milliseconds waitFor, bool allowBackground) {
  struct ConnectState {
    std::mutex mutex;
    std::condition_variable cv;
    bool finished = false;
    bool success = false;
    std::atomic<bool> backgroundAnnounce{false};
  };

  auto state = std::make_shared<ConnectState>();

  auto worker = std::thread([&network, ip, port, allowBackground, state]() {
    bool ok = network.connectToNode(ip, port);
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      state->finished = true;
      state->success = ok;
    }
    state->cv.notify_one();
    if (allowBackground && state->backgroundAnnounce.load()) {
      std::lock_guard<std::mutex> outLock(outputMutex);
      if (ok) {
        std::cout << "✅ Connected to peer " << ip << ':' << port << std::endl;
      } else {
        std::cout << "❌ Could not connect to peer: " << ip << ':' << port
                  << std::endl;
      }
    }
  });

  std::unique_lock<std::mutex> lock(state->mutex);
  if (waitFor.count() > 0) {
    if (!state->cv.wait_for(lock, waitFor,
                            [&]() { return state->finished; })) {
      if (allowBackground) {
        state->backgroundAnnounce.store(true);
        lock.unlock();
        worker.detach();
        return ConnectOutcome::Pending;
      }
    }
  }

  if (!state->finished) {
    state->cv.wait(lock, [&]() { return state->finished; });
  }
  lock.unlock();

  if (worker.joinable())
    worker.join();

  return state->success ? ConnectOutcome::Success : ConnectOutcome::Failure;
}

} // namespace alyn::cli
