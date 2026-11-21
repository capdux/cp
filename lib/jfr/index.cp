// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc JVM Java Flight Recorder v2.0 Parser)

export {
    JFR_MAGIC,
    JFR_CHUNK_HEADER_SIZE,
    JFR_EVENT_METADATA,
    JFR_EVENT_CHECKPOINT,
    JFR_FLAG_COMPRESSED_INTS,
    JFR_FLAG_FINAL_CHUNK,
    JfrChunkHeader,
    JfrChunk,
    JfrVarint,
    JfrEventFrame,
    JfrStats,
    parseJfrChunkHeader,
    isJfr,
    readJfrUleb128,
    JfrChunkIter,
    JfrEventIter,
    summarizeJfr,
} from "./jfr.cp";
