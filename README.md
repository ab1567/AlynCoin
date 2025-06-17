# AlynCoin
Privacy-focused cryptocurrency based on PoW

## Aggregated Proof Sync

Starting with this version, nodes default to "small proof" synchronization.  The
`g_enableAggProof` flag is on by default, so peers exchange epoch headers and
STARK proofs instead of the entire blockchain.

You can opt out and force legacy full-chain syncing by passing the
`--disable-agg-proof` flag when starting the node.  Use `--enable-agg-proof` to
explicitly enable it if running with a custom configuration.
