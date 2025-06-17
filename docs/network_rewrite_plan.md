# Network Refactor Plan

This document tracks the staged effort to replace the existing `network.cpp`
implementation with a modular networking layer based on the `INetwork`
interface.

## Batch 1 – Skeleton
- [x] Introduce the `INetwork` interface defining the minimal transport API.
- [x] Add `LegacyNetwork` wrapping the current `Network` class.
- [x] Add `StarkNetNetwork` skeleton for the future QUIC/STARK implementation.
- [x] Provide a simple factory (`make_network`) selecting an implementation.
- [x] Update `src/CMakeLists.txt` to compile the new files.

## Batch 2 – Bridging to old code
- [x] Implement the methods of `LegacyNetwork` by delegating to the existing
  `Network` class from `network.cpp`.
- [x] Expose configuration flags so the node can select legacy or starknet mode.

## Batch 3 – StarkNetNetwork I/O
- [ ] Implement basic QUIC transport setup and message framing.
- [ ] Support block broadcast and block request with stub handlers.
- [ ] Hook callbacks for block arrival through the `onBlock` subscription.

## Batch 4 – Epoch sync and proofs
- [ ] Serialize recursive STARK proofs together with block headers.
- [ ] Add epoch proof messages and state snapshot download logic.
- [ ] Integrate with the prover/verifier components.

## Batch 5 – Remove legacy path
- [ ] After extensive testing, make `StarkNetNetwork` the default.
- [ ] Retire the old `network.cpp` and associated headers.
