// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RIFF/RIFX Container Chunk Parser)

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
