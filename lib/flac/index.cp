// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc FLAC Audio Metadata Parser)

export {
    FLAC_MAGIC_SIZE,
    FLAC_BLOCK_HEADER_SIZE,
    FLAC_STREAMINFO_SIZE,
    FlacBlockType,
    FlacMetadataBlock,
    FlacStreamInfo,
    isFlac,
    FlacBlockIter,
    parseStreamInfoPayload,
    parseFlacStreamInfo,
} from "./flac.cp";
