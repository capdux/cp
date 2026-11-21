// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ISO 28500 WARC 1.0/1.1 Parser)
// Unified exports for cap:warc.

export {
    WARC_VERSION_1_0,
    WARC_VERSION_1_1,
    WarcRecordType,
    warcRecordTypeFromSlice,
    WarcField,
    WarcFieldIter,
    findWarcHeader,
    WarcRecord,
    WarcIter,
    WarcStats,
    isWarc,
    summarizeWarc,
} from "./warc.cp";
