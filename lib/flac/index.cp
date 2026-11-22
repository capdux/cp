// Origin: Pure-CP Stdlib (Zero-Alloc FLAC Audio Metadata Parser) | Created: 2026-11-22

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
