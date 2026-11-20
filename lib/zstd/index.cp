// Source: cp/lib/zstd/index.cp (Unified Exports for cap:zstd)
// Pure CP RFC 8878 Zstandard Frame & Block Codec with XXH64 checksums.

export {
    ZstdError,
    ZstdFrameInfo,
    ZSTD_MAGIC,
    ZSTD_SKIPPABLE_MAGIC_BASE,
    ZSTD_MAX_BLOCK_SIZE,
    xxhash64,
    inspectFrameHeader,
    compress,
    decompress,
    decompressWithLimits,
} from "./zstd.cp";
