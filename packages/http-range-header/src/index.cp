// @cap/http-range-header — Zero-heap RFC 9110 HTTP Range header parser & validator
// Clean-room pure CP counterpart to crates.io:http-range-header
// Accelerated with 16-lane @Vector(16, u8) SIMD delimiter/whitespace scanning, comptime 256-entry digit LUT, @memcpy, and undefined stack buffers.

import { eqIgnoreCase } from "cap:ascii";

type HrhVec16 = @Vector(16, u8);

fn hrhBuildDigitLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = 0xFF;
        i += 1;
    }
    let d: usize = 0;
    while (d < 10) {
        lut[48 + d] = d as u8;
        d += 1;
    }
    return lut;
}

const HRH_DIGIT_LUT: [256]u8 = hrhBuildDigitLut();

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

fn hrhSkipSpaces(s: string, start: usize, end: usize): usize {
    let i = start;
    const v_sp: HrhVec16 = @splat(0x20 as u8);
    const v_tb: HrhVec16 = @splat(0x09 as u8);
    while (i + 16 <= end) {
        const v: HrhVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v == v_sp) | (v == v_tb))) {
            break;
        }
        i += 16;
    }
    while (i < end && hrhIsSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn hrhFindByte(s: string, start: usize, end: usize, target: u8): usize {
    let i = start;
    const v_t: HrhVec16 = @splat(target);
    while (i + 16 <= end) {
        const v: HrhVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_t)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < end) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return end;
}

fn hrhParseU64(s: string, start: usize, end: usize): u64? {
    if (start >= end) {
        return null;
    }
    const max_div10: u64 = 1844674407370955161;
    const max_mod10: u64 = 5;
    let acc: u64 = 0;
    let i = start;
    while (i < end) {
        const d = HRH_DIGIT_LUT[s[i] as usize];
        if (d > 9) {
            return null;
        }
        const du = d as u64;
        if (acc > max_div10 || (acc == max_div10 && du > max_mod10)) {
            return null;
        }
        acc = acc * 10 + du;
        i += 1;
    }
    return acc;
}

fn hrhWriteSlice(src: string, out: []u8, pos: usize): usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const copy_len = if (src.len < avail) src.len else avail;
    @memcpy(out[pos .. pos + copy_len], src[0..copy_len]);
    return pos + copy_len;
}

fn hrhWriteU64(val: u64, out: []u8, pos: usize): usize {
    if (pos >= out.len) {
        return pos;
    }
    let tmp: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        out[pos] = 48;
        return pos + 1;
    }
    while (v > 0) {
        tmp[23 - len] = (48 + (v % 10)) as u8;
        v = v / 10;
        len += 1;
    }
    const avail = out.len - pos;
    const copy_len = if (len < avail) len else avail;
    const start_idx = 24 - len;
    @memcpy(out[pos .. pos + copy_len], tmp[start_idx .. start_idx + copy_len]);
    return pos + copy_len;
}

/// Parse an RFC 9110 `Range` header value (`bytes=...`) into `out`.
/// Returns the number of parsed syntactic ranges, or `0` if the header is malformed.
pub fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize {
    let start: usize = hrhSkipSpaces(header, 0, header.len);
    let end = header.len;
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
        i = hrhSkipSpaces(header, i, end);
        if (i >= end) {
            // Trailing comma or empty range-set
            return 0;
        }

        const itemEnd = hrhFindByte(header, i, end, 0x2C);

        let segStart = i;
        let segEnd = itemEnd;
        while (segEnd > segStart && hrhIsSpace(header[segEnd - 1])) {
            segEnd -= 1;
        }
        if (segStart >= segEnd) {
            return 0;
        }

        // Find '-' inside [segStart, segEnd)
        const dash = hrhFindByte(header, segStart, segEnd, 0x2D);
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
    pos = hrhWriteSlice("bytes ", out, pos);
    pos = hrhWriteU64(start, out, pos);
    pos = hrhWriteSlice("-", out, pos);
    pos = hrhWriteU64(end, out, pos);
    pos = hrhWriteSlice("/", out, pos);
    pos = hrhWriteU64(total, out, pos);
    return pos;
}

/// Format an RFC 9110 416 unsatisfiable `Content-Range` header value: `bytes */<total>`.
pub fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize {
    let pos: usize = 0;
    pos = hrhWriteSlice("bytes */", out, pos);
    pos = hrhWriteU64(total, out, pos);
    return pos;
}
