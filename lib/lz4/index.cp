// Source: cp/lib/lz4/index.cp (Unified Exports for cap:lz4)
// Pure CP zero-C-dependency LZ4 Block & Frame compression engine with XXH32 checksums.

export {
    Lz4Error,
    LZ4_FRAME_MAGIC,
    LZ4_FRAME_BLOCK_SIZE,
    xxhash32,
    Lz4Block,
    Lz4Frame,
} from "./lz4.cp";
