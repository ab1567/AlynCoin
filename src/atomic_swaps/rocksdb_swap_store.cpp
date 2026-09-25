#include "rocksdb_swap_store.h"
#include "../db/rocksdb_options_utils.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/write_batch.h>
#include "proto_utils.h"  // serializeSwap, deserializeSwap

RocksDBAtomicSwapStore::RocksDBAtomicSwapStore(const std::string& dbPath) {
    options.create_if_missing = true;
    alyn::db::ApplyDatabaseDefaults(options);
    rocksdb::Status status = rocksdb::TransactionDB::Open(options, txn_options, dbPath, &db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
    }
}

RocksDBAtomicSwapStore::~RocksDBAtomicSwapStore() {
    delete db;
}

bool RocksDBAtomicSwapStore::saveSwap(const AtomicSwap& swap) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string key = makeKey(swap.uuid);
    std::string value;
    if (!serializeSwap(swap, value)) return false;
    return db->Put(rocksdb::WriteOptions(), key, value).ok();
}

bool RocksDBAtomicSwapStore::updateSwap(const AtomicSwap& swap) {
    return saveSwap(swap); // alias for clarity
}

std::optional<AtomicSwap> RocksDBAtomicSwapStore::loadSwap(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), makeKey(uuid), &value);
    if (!status.ok()) return std::nullopt;

    AtomicSwap swap;
    if (!deserializeSwap(value, swap)) return std::nullopt;
    return swap;
}
