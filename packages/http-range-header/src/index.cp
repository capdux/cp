// @cap/http-range-header — Zero-heap RFC 9110 HTTP Range header parser & validator
// Clean-room pure CP counterpart to crates.io:http-range-header

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

const HRH_MAX_U64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

fn hrhIsSpace(b: u8): bool {
    return b == 0x20 || b == 0x09;
}

fn hrhIsDigit(b: u8): bool {
    return b >= 0x30 && b <= 0x39;
}

fn hrhToLower(b: u8): u8 {
    if (b >= 0x41 && b <= 0x5A) {
        return b + 32;
    }
    return b;
}

fn hrhHasBytesPrefix(s: string, start: usize, end: usize): bool {
    if (end - start < 6) {
        return false;
    }
    return hrhToLower(s[start]) == 0x62 &&
        hrhToLower(s[start + 1]) == 0x79 &&
        hrhToLower(s[start + 2]) == 0x74 &&
        hrhToLower(s[start + 3]) == 0x65 &&
        hrhToLower(s[start + 4]) == 0x73 &&
        s[start + 5] == 0x3D;
}

fn hrhParseU64(s: string, start: usize, end: usize): u64? {
    if (start >= end) {
        return null;
    }
    let val: u64 = 0;
    let i = start;
    while (i < end) {
        if (!hrhIsDigit(s[i])) {
            return null;
        }
        const d = (s[i] - 0x30) as u64;
        if (val > (HRH_MAX_U64 - d) / 10) {
            return null;
        }
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn hrhWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn hrhWriteSlice(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn hrhWriteU64(out: []u8, pos: usize, val: u64): usize {
    if (val == 0) {
        return hrhWriteByte(out, pos, 0x30);
    }
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    let p = pos;
    while (n > 0) {
        n -= 1;
        p = hrhWriteByte(out, p, tmp[n]);
    }
    return p;
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
    if (!hrhHasBytesPrefix(header, start, end)) {
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

        let itemEnd = i;
        while (itemEnd < end && header[itemEnd] != 0x2C) {
            itemEnd += 1;
        }

        let segStart = i;
        let segEnd = itemEnd;
        while (segEnd > segStart && hrhIsSpace(header[segEnd - 1])) {
            segEnd -= 1;
        }
        if (segStart >= segEnd) {
            return 0;
        }

        // Find '-' inside [segStart, segEnd)
        let dash: usize = segEnd;
        let k = segStart;
        while (k < segEnd) {
            if (header[k] == 0x2D) {
                dash = k;
                break;
            }
            k += 1;
        }
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
    pos = hrhWriteSlice(out, pos, "bytes ");
    pos = hrhWriteU64(out, pos, start);
    pos = hrhWriteByte(out, pos, 0x2D);
    pos = hrhWriteU64(out, pos, end);
    pos = hrhWriteByte(out, pos, 0x2F);
    pos = hrhWriteU64(out, pos, total);
    return pos;
}

/// Format an RFC 9110 416 unsatisfiable `Content-Range` header value: `bytes */<total>`.
pub fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize {
    let pos: usize = 0;
    pos = hrhWriteSlice(out, pos, "bytes */");
    pos = hrhWriteU64(out, pos, total);
    return pos;
}
