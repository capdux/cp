// @cap/range-parser — Zero-allocation RFC 9110 HTTP Range Header Parser & Content-Range Formatter
// Clean-room MIT implementation in pure CP.

pub const MAX_RANGES: usize = 16;
pub const RANGE_OK: i32 = 0;
pub const RANGE_UNSATISFIABLE: i32 = -1;
pub const RANGE_MALFORMED: i32 = -2;

pub struct ByteRange {
    pub start: u64,
    pub end: u64,
}

pub struct ParseRangeResult {
    pub status: i32,
    pub count: usize,
    pub ranges: [16]ByteRange,
}

struct ParsedU64 {
    val: u64,
    ok: bool,
}

fn emptyResult(status: i32) -> ParseRangeResult {
    let ranges: [16]ByteRange = undefined;
    let i: usize = 0;
    while (i < MAX_RANGES) {
        ranges[i] = ByteRange { start: 0, end: 0 };
        i += 1;
    }
    return ParseRangeResult {
        status: status,
        count: 0,
        ranges: ranges,
    };
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn parseU64Slice(data: []const u8, start: usize, end: usize) -> ParsedU64 {
    if (start >= end) {
        return ParsedU64 { val: 0, ok: false };
    }
    const max_div10: u64 = 1844674407370955161;
    let acc: u64 = 0;
    let i = start;
    while (i < end) {
        const ch = data[i];
        if (ch < 48 || ch > 57) {
            return ParsedU64 { val: 0, ok: false };
        }
        const digit = (ch - 48) as u64;
        if (acc > max_div10 || (acc == max_div10 && digit > 5)) {
            return ParsedU64 { val: 0, ok: false };
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return ParsedU64 { val: acc, ok: true };
}

/// Parses an RFC 9110 HTTP `Range` header (e.g. `"bytes=0-499, 1000-"` or `"bytes=-200"`).
/// When `combine` is true, overlapping and adjacent byte ranges are sorted and merged.
pub fn parseRange(size: u64, header: []const u8, combine: bool) -> ParseRangeResult {
    let pos: usize = 0;
    let hlen = header.len;
    while (pos < hlen && isSpace(header[pos])) {
        pos += 1;
    }
    while (hlen > pos && isSpace(header[hlen - 1])) {
        hlen -= 1;
    }

    // Must start with "bytes=" (case-insensitive)
    if (hlen - pos < 6) {
        return emptyResult(RANGE_MALFORMED);
    }
    if (toLowerAscii(header[pos]) != 98
        || toLowerAscii(header[pos + 1]) != 121
        || toLowerAscii(header[pos + 2]) != 116
        || toLowerAscii(header[pos + 3]) != 101
        || toLowerAscii(header[pos + 4]) != 115
        || header[pos + 5] != 61)
    {
        return emptyResult(RANGE_MALFORMED);
    }

    pos += 6;
    if (pos >= hlen) {
        return emptyResult(RANGE_MALFORMED);
    }

    let res = emptyResult(RANGE_OK);
    let count: usize = 0;

    while (pos <= hlen) {
        const seg_start = pos;
        while (pos < hlen && header[pos] != 44) {
            pos += 1; // ','
        }
        const seg_end = pos;

        // Trim whitespace around each range spec
        let s = seg_start;
        let e = seg_end;
        while (s < e && isSpace(header[s])) {
            s += 1;
        }
        while (e > s && isSpace(header[e - 1])) {
            e -= 1;
        }
        if (s >= e) {
            return emptyResult(RANGE_MALFORMED);
        }

        // Find '-'
        let dash: usize = e;
        let k = s;
        while (k < e) {
            if (header[k] == 45) {
                dash = k;
                break;
            }
            k += 1;
        }
        if (dash == e) {
            return emptyResult(RANGE_MALFORMED);
        }

        const has_start = dash > s;
        const has_end = (dash + 1) < e;
        if (!has_start && !has_end) {
            return emptyResult(RANGE_MALFORMED);
        }

        if (!has_start && has_end) {
            // Suffix range: "-N"
            const parsed_suf = parseU64Slice(header, dash + 1, e);
            if (!parsed_suf.ok) {
                return emptyResult(RANGE_MALFORMED);
            }
            if (size > 0 && parsed_suf.val > 0) {
                let r_start: u64 = 0;
                if (parsed_suf.val < size) {
                    r_start = size - parsed_suf.val;
                }
                const r_end = size - 1;
                if (count < MAX_RANGES) {
                    res.ranges[count] = ByteRange { start: r_start, end: r_end };
                    count += 1;
                }
            }
        } else if (has_start && !has_end) {
            // Open-ended range: "N-"
            const parsed_s = parseU64Slice(header, s, dash);
            if (!parsed_s.ok) {
                return emptyResult(RANGE_MALFORMED);
            }
            if (size > 0 && parsed_s.val < size) {
                if (count < MAX_RANGES) {
                    res.ranges[count] = ByteRange { start: parsed_s.val, end: size - 1 };
                    count += 1;
                }
            }
        } else {
            // Closed range: "A-B"
            const parsed_s = parseU64Slice(header, s, dash);
            const parsed_e = parseU64Slice(header, dash + 1, e);
            if (!parsed_s.ok || !parsed_e.ok) {
                return emptyResult(RANGE_MALFORMED);
            }
            if (size > 0 && parsed_s.val <= parsed_e.val && parsed_s.val < size) {
                let r_end = parsed_e.val;
                if (r_end >= size) {
                    r_end = size - 1;
                }
                if (count < MAX_RANGES) {
                    res.ranges[count] = ByteRange { start: parsed_s.val, end: r_end };
                    count += 1;
                }
            }
        }

        if (pos == hlen) {
            break;
        }
        pos += 1; // skip ','
    }

    if (count == 0) {
        return emptyResult(RANGE_UNSATISFIABLE);
    }

    if (combine && count > 1) {
        // Insertion sort by (start, end)
        let i: usize = 1;
        while (i < count) {
            const key = res.ranges[i];
            let j = i;
            while (j > 0) {
                const prev = res.ranges[j - 1];
                const swap = (prev.start > key.start) || (prev.start == key.start && prev.end > key.end);
                if (!swap) {
                    break;
                }
                res.ranges[j] = prev;
                j -= 1;
            }
            res.ranges[j] = key;
            i += 1;
        }

        // Merge overlapping or adjacent ranges
        let out_idx: usize = 0;
        let m: usize = 1;
        while (m < count) {
            const cur = res.ranges[out_idx];
            const nxt = res.ranges[m];
            const touches = (nxt.start <= cur.end) || (nxt.start - cur.end == 1);
            if (touches) {
                if (nxt.end > cur.end) {
                    res.ranges[out_idx].end = nxt.end;
                }
            } else {
                out_idx += 1;
                res.ranges[out_idx] = nxt;
            }
            m += 1;
        }
        count = out_idx + 1;
    }

    res.status = RANGE_OK;
    res.count = count;
    return res;
}

fn writeBytes(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn writeU64(dst: []u8, pos: usize, val: u64) -> i64 {
    let tmp: [20]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    if (pos + n > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < n) {
        dst[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return (pos + n) as i64;
}

/// Formats `"bytes <start>-<end>/<total_size>"` into `dst`.
/// Returns the number of bytes written, or `-1` if `dst` is too small.
pub fn formatContentRange(start: u64, end: u64, total_size: u64, dst: []u8) -> i64 {
    let p = writeBytes(dst, 0, "bytes ");
    if (p < 0) {
        return -1;
    }
    p = writeU64(dst, p as usize, start);
    if (p < 0) {
        return -1;
    }
    p = writeBytes(dst, p as usize, "-");
    if (p < 0) {
        return -1;
    }
    p = writeU64(dst, p as usize, end);
    if (p < 0) {
        return -1;
    }
    p = writeBytes(dst, p as usize, "/");
    if (p < 0) {
        return -1;
    }
    return writeU64(dst, p as usize, total_size);
}

/// Formats `"bytes */<total_size>"` into `dst` (for HTTP 416 responses).
/// Returns the number of bytes written, or `-1` if `dst` is too small.
pub fn formatUnsatisfiableRange(total_size: u64, dst: []u8) -> i64 {
    let p = writeBytes(dst, 0, "bytes */");
    if (p < 0) {
        return -1;
    }
    return writeU64(dst, p as usize, total_size);
}
