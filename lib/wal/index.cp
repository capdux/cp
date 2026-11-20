// Source: cp/lib/wal/index.cp (Unified Exports for cap:wal)
// Pure CP zero-C-dependency CRC32C Write-Ahead Log framing, recovery & compaction engine.

export {
    WAL_MAGIC,
    WAL_HEADER_SIZE,
    WAL_DEFAULT_MAX_RECORD_BYTES,
    WalError,
    WalRecordType,
    WalRecord,
    WalRecoveryResult,
    WalLog,
    crc32c,
} from "./wal.cp";
