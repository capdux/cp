// Source: Absorbed from /data/dev/bun/src/http/SendFile.cp | Synced: 2026-11-22T11:12:13Z (RFC 9110 §14 HTTP Range / Content-Range)
pub import {
    ByteRange,
    RangeStatus,
    ParsedRangeHeader,
    parseRangeHeader,
    formatContentRange,
    formatUnsatisfiedRange,
    ParsedContentRange,
    parseContentRange,
} from "./range.cp";
