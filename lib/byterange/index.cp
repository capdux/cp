// Source: Absorbed from /data/dev/bun/src/runtime/server/RangeRequest.cp | Synced: 2026-11-22T09:28:26Z
// Backward-compatible re-export: canonical implementation consolidated into lib/http/byterange.cp

export {
    ByteRange,
    RangeStatus,
    RangeResult,
    RawRangeKind,
    RawRange,
    MultiRangeResult,
    ParsedContentRange,
    parseRaw,
    parseRange,
    parseMultiRange,
    formatContentRange,
    formatUnsatisfiableContentRange,
    parseContentRange,
} from "../http/byterange.cp";
