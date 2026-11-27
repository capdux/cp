// @cap/http-range-header — Zero-heap RFC 9110 HTTP Range header parser & validator
// Clean-room pure CP counterpart to crates.io:http-range-header

import { eqIgnoreCase, findByte } from "cap:ascii";
import { parseU64, writeStr, writeU64 } from "cap:fmt";

pub enum RangePosition {
    StartAndEnd,
    StartOnly,
    Suffix,
}

pub struct SyntacticByteRange {
    kind: RangePosition,
    start: u64,
    end: u64,
}

pub struct ValidatedByteRange {
    start: u64,
    end: u64,

    fn byteLen(): u64 {
        return _.end - _.start + 1;
    }
}

fn hrhIsSpace(b: u8): bool {
    return b == 0x20 || b == 0x09;
}

fn hrhParseU64(s: string, start: usize, end: usize): u64? {
    if (start >= end || s[start] == 43) {
        return null;
    }
    return parseU64(s[start..end]);
}

/// Parse an RFC 9110 `Range` header value (`bytes=...`) into `out`.
/// Returns the number of parsed syntactic ranges, or `0` if the header is malformed.
pub fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize {
    let start: usize = 0;
    let end = header.len;
    while (start < end && hrhIsSpace(header[start])) {
        start += 1;
    }
    while (end > start && hrhIsSpace(header[end - 1])) {
        end -= 1;
    }
    if (end - start < 6 || !eqIgnoreCase(header[start..(start + 6)], "bytes=")) {
        return 0;
    }

    let i = start + 6;
    let count: usize = 0;
    let expectItem = true;

    while (i < end || expectItem) {
        while (i < end && hrhIsSpace(header[i])) {
            i += 1;
        }
        if (i >= end) {
            // Trailing comma or empty range-set
            return 0;
        }

        const itemEnd = findByte(header[0..end], 0x2C, i) ?? end;

        let segStart = i;
        let segEnd = itemEnd;
        while (segEnd > segStart && hrhIsSpace(header[segEnd - 1])) {
            segEnd -= 1;
        }
        if (segStart >= segEnd) {
            return 0;
        }

        // Find '-' inside [segStart, segEnd)
        const dash = findByte(header[0..segEnd], 0x2D, segStart) ?? segEnd;
        if (dash == segEnd) {
            return 0;
        }

        let parsedRange = SyntacticByteRange{ kind: RangePosition.StartOnly, start: 0, end: 0 };
        if (dash == segStart) {
            // Suffix range: -<suffix>
            const sufOpt = hrhParseU64(header, dash + 1, segEnd);
            if (suf := sufOpt) {
                parsedRange = SyntacticByteRange{
                    kind: RangePosition.Suffix,
                    start: 0,
                    end: suf,
                };
            } else {
                return 0;
            }
        } else if (dash + 1 == segEnd) {
            // Open-ended range: <start>-
            const stOpt = hrhParseU64(header, segStart, dash);
            if (st := stOpt) {
                parsedRange = SyntacticByteRange{
                    kind: RangePosition.StartOnly,
                    start: st,
                    end: 0,
                };
            } else {
                return 0;
            }
        } else {
            // Closed range: <start>-<end>
            const stOpt = hrhParseU64(header, segStart, dash);
            const enOpt = hrhParseU64(header, dash + 1, segEnd);
            let stVal: u64 = 0;
            let enVal: u64 = 0;
            if (st := stOpt) {
                stVal = st;
            } else {
                return 0;
            }
            if (en := enOpt) {
                enVal = en;
            } else {
                return 0;
            }
            if (enVal < stVal) {
                return 0;
            }
            parsedRange = SyntacticByteRange{
                kind: RangePosition.StartAndEnd,
                start: stVal,
                end: enVal,
            };
        }

        if (count < out.len) {
            out[count] = parsedRange;
            count += 1;
        }

        if (itemEnd < end && header[itemEnd] == 0x2C) {
            i = itemEnd + 1;
            expectItem = true;
        } else {
            i = itemEnd;
            expectItem = false;
        }
    }

    return count;
}

/// Validate syntactic byte ranges against a representation of `total_size` bytes.
/// Clamps overflowing end offsets to `total_size - 1` and resolves suffix ranges.
/// Returns the number of valid ranges written into `out`, or `-1` if unsatisfiable (HTTP 416).
pub fn validateRanges(ranges: []const SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64 {
    if (ranges.len == 0 || total_size == 0) {
        return -1;
    }

    let validCount: usize = 0;
    let i: usize = 0;
    while (i < ranges.len) {
        const r = ranges[i];
        if (r.kind == RangePosition.StartAndEnd) {
            if (r.start < total_size && r.start <= r.end) {
                const clampedEnd = if (r.end >= total_size) total_size - 1 else r.end;
                if (validCount < out.len) {
                    out[validCount] = ValidatedByteRange{ start: r.start, end: clampedEnd };
                    validCount += 1;
                }
            }
        } else if (r.kind == RangePosition.StartOnly) {
            if (r.start < total_size) {
                if (validCount < out.len) {
                    out[validCount] = ValidatedByteRange{ start: r.start, end: total_size - 1 };
                    validCount += 1;
                }
            }
        } else {
            // Suffix range: last `r.end` bytes
            if (r.end > 0) {
                const startOff = if (r.end >= total_size) (0 as u64) else (total_size - r.end);
                if (validCount < out.len) {
                    out[validCount] = ValidatedByteRange{ start: startOff, end: total_size - 1 };
                    validCount += 1;
                }
            }
        }
        i += 1;
    }

    if (validCount == 0) {
        return -1;
    }
    return validCount as i64;
}

/// Format an RFC 9110 `Content-Range` header value: `bytes <start>-<end>/<total>`.
pub fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize {
    let pos: usize = 0;
    pos = writeStr("bytes ", out, pos);
    pos = writeU64(start, out, pos);
    pos = writeStr("-", out, pos);
    pos = writeU64(end, out, pos);
    pos = writeStr("/", out, pos);
    pos = writeU64(total, out, pos);
    return pos;
}

/// Format an RFC 9110 416 unsatisfiable `Content-Range` header value: `bytes */<total>`.
pub fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize {
    let pos: usize = 0;
    pos = writeStr("bytes */", out, pos);
    pos = writeU64(total, out, pos);
    return pos;
}
