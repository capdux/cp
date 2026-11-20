// Source: cp/lib/brotli/index.cp (Public Unified Export for cap:brotli & cap.brotli)

export {
    BrotliError,
    BrotliHeader,
    DEFAULT_WINDOW_BITS,
    MIN_WINDOW_BITS,
    MAX_WINDOW_BITS,
    MAX_META_BLOCK_LEN,
    DEFAULT_MAX_WINDOW_BYTES,
    DEFAULT_MAX_OUTPUT_BYTES,
    windowSizeFromBits,
    parseHeader,
    compress,
    compressUncompressed,
    decompress,
    decompressWithLimits,
} from "./brotli.cp";
