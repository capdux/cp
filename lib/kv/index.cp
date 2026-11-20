// Source: cp/lib/kv/index.cp (Unified Exports for cap:kv)
// Pure CP zero-C-dependency Bitcask-style durable embedded key-value store
// backed by CRC32 append-only WAL and atomic compaction.

export {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    KvError,
    WalOp,
    WalRecord,
    WalFrame,
    WalDecodeResult,
    WalCodec,
} from "./wal.cp";

export {
    KvStoreOptions,
    KvStats,
    KvStore,
} from "./store.cp";
