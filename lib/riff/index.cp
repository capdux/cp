// Origin: Pure-CP Stdlib (Zero-Alloc RIFF/RIFX Container Chunk Parser) | Created: 2026-11-22

export {
    RIFF_HEADER_SIZE,
    RIFF_CHUNK_HEADER_SIZE,
    RiffHeader,
    RiffChunk,
    fourccEquals,
    parseRiffHeader,
    parseListType,
    RiffChunkIter,
    findRiffChunk,
} from "./riff.cp";
