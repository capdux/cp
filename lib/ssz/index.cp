// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Ethereum SSZ Deterministic Codec)

export {
    SSZ_OFFSET_BYTES,
    SSZ_CHUNK_BYTES,
    SszWriter,
    SszReader,
    sszBitlistLen,
    sszBitlistGet,
    sszBitvectorValid,
    sszBitvectorGet,
    SszOffsetListIter,
    sszChunkCount,
    sszNextPowerOfTwo,
} from "./ssz.cp";
