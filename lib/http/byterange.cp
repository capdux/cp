// Source: Absorbed from /data/dev/bun/src/runtime/server/RangeRequest.cp | Synced: 2026-11-22T00:27:22Z

/// Inclusive byte range `[start, end]` where `start <= end`.
pub struct ByteRange {
    start: u64,
    end: u64,

    /// Returns the number of bytes covered by this inclusive range (`end - start + 1`).
    fn length(): u64 {
        if (_.end < _.start) {
            return 0;
        }
        return (_.end - _.start) + 1;
    }
}

/// Resolution outcome for an HTTP `Range` request against a known total resource length.
pub enum RangeStatus {
    /// No valid Range header (or unsupported form) — serve 200 with full body.
    None,
    /// Serve 206 Partial Content with `Content-Range: bytes start-end/total`.
    Satisfiable,
    /// Serve 416 Range Not Satisfiable with `Content-Range: bytes */total`.
    Unsatisfiable,
}

/// Single-range parse & resolve result (absorbed from Bun `RangeRequest.Result`).
pub struct RangeResult {
    status: RangeStatus,
    range: ByteRange,
}

/// Kind of un-resolved `RawRange` parsed before total resource length is known.
pub enum RawRangeKind {
    None,
    Suffix,
    Bounded,
}

/// Parsed single `Range` header before the total size is known (absorbed from Bun `RangeRequest.Raw`).
/// Owns no slices into the input buffer and is safe to store on a request context.
pub struct RawRange {
    kind: RawRangeKind,
    start: u64,
    end: u64?,
    suffix_len: u64,

    /// Resolves this `RawRange` against a known `total` byte length.
    fn resolve(total: u64): RangeResult {
        match (_.kind) {
            RawRangeKind.None => {
                return RangeResult {
                    status: RangeStatus.None,
                    range: ByteRange { start: 0, end: 0 },
                };
            }
            RawRangeKind.Suffix => {
                if (_.suffix_len == 0) {
                    return RangeResult {
                        status: RangeStatus.Unsatisfiable,
                        range: ByteRange { start: 0, end: 0 },
                    };
                }
                if (total == 0) {
                    return RangeResult {
                        status: RangeStatus.None,
                        range: ByteRange { start: 0, end: 0 },
                    };
                }
                const start_pos: u64 = if (_.suffix_len >= total) 0 else (total - _.suffix_len);
                return RangeResult {
                    status: RangeStatus.Satisfiable,
                    range: ByteRange { start: start_pos, end: total - 1 },
                };
            }
            RawRangeKind.Bounded => {
                if (_.start >= total) {
                    return RangeResult {
                        status: RangeStatus.Unsatisfiable,
                        range: ByteRange { start: 0, end: 0 },
                    };
                }
                let end_pos: u64 = _.end ?? (total - 1);
                if (end_pos < _.start) {
                    return RangeResult {
                        status: RangeStatus.None,
                        range: ByteRange { start: 0, end: 0 },
                    };
                }
                if (end_pos >= total) {
                    end_pos = total - 1;
                }
                return RangeResult {
                    status: RangeStatus.Satisfiable,
                    range: ByteRange { start: _.start, end: end_pos },
                };
            }
        }
    }
}

/// Outcome of `parseMultiRange`.
pub struct MultiRangeResult {
    status: RangeStatus,
    count: usize,
}

/// Parsed HTTP `Content-Range` response header (RFC 9110 §14.4).
pub struct ParsedContentRange {
    is_unsatisfied: bool,
    start: u64,
    end: u64,
    total: u64?,
}

fn toAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn startsWithBytesKeyword(s: []const u8): bool {
    if (s.len < 5) {
        return false;
    }
    return toAsciiLower(s[0]) == 'b' &&
        toAsciiLower(s[1]) == 'y' &&
        toAsciiLower(s[2]) == 't' &&
        toAsciiLower(s[3]) == 'e' &&
        toAsciiLower(s[4]) == 's';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn findByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseU64Decimal(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = (c - '0') as u64;
        if (acc > (0xFFFFFFFFFFFFFFFF - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn formatU64Decimal(val: u64, out_buf: *[]u8, pos: *usize): bool {
    let tmp: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            len += 1;
        }
    }
    const p = pos.*;
    if (out_buf.*.len - p < len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        out_buf.*[p + i] = tmp[len - 1 - i];
        i += 1;
    }
    pos.* = p + len;
    return true;
}

fn writeLit(lit: []const u8, out_buf: *[]u8, pos: *usize): bool {
    const p = pos.*;
    if (out_buf.*.len - p < lit.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        out_buf.*[p + i] = lit[i];
        i += 1;
    }
    pos.* = p + lit.len;
    return true;
}

fn parseSingleSpec(spec: []const u8): RawRange {
    const dash = findByte(spec, '-') ?? return RawRange {
        kind: RawRangeKind.None,
        start: 0,
        end: null,
        suffix_len: 0,
    };
    const start_s = trimSpaces(spec[0..dash]);
    const end_s = trimSpaces(spec[dash + 1..]);

    if (start_s.len == 0) {
        const n = parseU64Decimal(end_s) ?? return RawRange {
            kind: RawRangeKind.None,
            start: 0,
            end: null,
            suffix_len: 0,
        };
        return RawRange {
            kind: RawRangeKind.Suffix,
            start: 0,
            end: null,
            suffix_len: n,
        };
    }

    const start_val = parseU64Decimal(start_s) ?? return RawRange {
        kind: RawRangeKind.None,
        start: 0,
        end: null,
        suffix_len: 0,
    };
    let end_opt: u64? = null;
    if (end_s.len > 0) {
        const ev = parseU64Decimal(end_s) ?? return RawRange {
            kind: RawRangeKind.None,
            start: 0,
            end: null,
            suffix_len: 0,
        };
        end_opt = ev;
    }
    return RawRange {
        kind: RawRangeKind.Bounded,
        start: start_val,
        end: end_opt,
        suffix_len: 0,
    };
}

/// Parses an HTTP `Range: bytes=...` header matching WebKit's `parseRange` / Bun `RangeRequest.parseRaw`
/// (WHATWG Fetch `#simple-range-header-value`: case-insensitive `"bytes"`, optional whitespace before `"="`).
/// Multi-range headers containing `,` fall back to `RawRangeKind.None` (use `parseMultiRange` for multi-range).
pub fn parseRaw(header: []const u8): RawRange {
    const none_val = RawRange { kind: RawRangeKind.None, start: 0, end: null, suffix_len: 0 };
    if (!startsWithBytesKeyword(header)) {
        return none_val;
    }
    const after_unit = trimSpaces(header[5..]);
    if (after_unit.len == 0 || after_unit[0] != '=') {
        return none_val;
    }
    const body = after_unit[1..];
    if (findByte(body, ',') != null) {
        return none_val;
    }
    return parseSingleSpec(body);
}

/// Parses and resolves a single HTTP `Range: bytes=...` header against known `total` resource length.
pub fn parseRange(header: []const u8, total: u64): RangeResult {
    return parseRaw(header).resolve(total);
}

/// Parses a comma-separated RFC 9110 §14.1.2 `Range: bytes=r1, r2, ...` header zero-alloc into `out_ranges`.
/// Returns `Satisfiable` with the number of satisfiable ranges written, `Unsatisfiable` if all ranges were
/// syntactically valid but unsatisfiable, or `None` on malformed syntax / non-`bytes` unit / buffer overflow.
pub fn parseMultiRange(header: []const u8, total: u64, out_ranges: *[]ByteRange): MultiRangeResult {
    const none_res = MultiRangeResult { status: RangeStatus.None, count: 0 };
    if (!startsWithBytesKeyword(header)) {
        return none_res;
    }
    const after_unit = trimSpaces(header[5..]);
    if (after_unit.len == 0 || after_unit[0] != '=') {
        return none_res;
    }
    const body = after_unit[1..];
    if (body.len == 0) {
        return none_res;
    }

    let count: usize = 0;
    let saw_unsatisfiable: bool = false;
    let cursor: usize = 0;

    while (cursor <= body.len) {
        let next_comma: usize = body.len;
        let k: usize = cursor;
        while (k < body.len) {
            if (body[k] == ',') {
                next_comma = k;
                break;
            }
            k += 1;
        }
        const spec = trimSpaces(body[cursor..next_comma]);
        const raw = parseSingleSpec(spec);
        if (raw.kind == RawRangeKind.None) {
            return none_res;
        }
        const res = raw.resolve(total);
        if (res.status == RangeStatus.None) {
            return none_res;
        } else if (res.status == RangeStatus.Unsatisfiable) {
            saw_unsatisfiable = true;
        } else {
            if (count >= out_ranges.*.len) {
                return none_res;
            }
            out_ranges.*[count] = res.range;
            count += 1;
        }

        if (next_comma == body.len) {
            break;
        }
        cursor = next_comma + 1;
    }

    if (count > 0) {
        return MultiRangeResult { status: RangeStatus.Satisfiable, count: count };
    }
    if (saw_unsatisfiable) {
        return MultiRangeResult { status: RangeStatus.Unsatisfiable, count: 0 };
    }
    return none_res;
}

/// Formats `Content-Range: bytes <start>-<end>/<total>` into `out_buf` (RFC 9110 §14.4).
pub fn formatContentRange(range: ByteRange, total: u64, out_buf: *[]u8): usize? {
    if (range.end < range.start || range.end >= total) {
        return null;
    }
    let pos: usize = 0;
    if (!writeLit("bytes ", out_buf, &pos)) return null;
    if (!formatU64Decimal(range.start, out_buf, &pos)) return null;
    if (!writeLit("-", out_buf, &pos)) return null;
    if (!formatU64Decimal(range.end, out_buf, &pos)) return null;
    if (!writeLit("/", out_buf, &pos)) return null;
    if (!formatU64Decimal(total, out_buf, &pos)) return null;
    return pos;
}

/// Formats `Content-Range: bytes */<total>` for HTTP 416 responses into `out_buf`.
pub fn formatUnsatisfiableContentRange(total: u64, out_buf: *[]u8): usize? {
    let pos: usize = 0;
    if (!writeLit("bytes */", out_buf, &pos)) return null;
    if (!formatU64Decimal(total, out_buf, &pos)) return null;
    return pos;
}

/// Parses an RFC 9110 §14.4 `Content-Range` header (`bytes start-end/total`, `bytes start-end/*`, or `bytes */total`).
pub fn parseContentRange(header: []const u8): ParsedContentRange? {
    const trimmed = trimSpaces(header);
    if (!startsWithBytesKeyword(trimmed) || trimmed.len < 7 || trimmed[5] != ' ') {
        return null;
    }
    const rest = trimSpaces(trimmed[6..]);
    const slash = findByte(rest, '/') ?? return null;
    const range_part = trimSpaces(rest[0..slash]);
    const total_part = trimSpaces(rest[slash + 1..]);

    if (range_part.len == 1 && range_part[0] == '*') {
        const tot = parseU64Decimal(total_part) ?? return null;
        return ParsedContentRange {
            is_unsatisfied: true,
            start: 0,
            end: 0,
            total: tot,
        };
    }

    const dash = findByte(range_part, '-') ?? return null;
    const start_val = parseU64Decimal(trimSpaces(range_part[0..dash])) ?? return null;
    const end_val = parseU64Decimal(trimSpaces(range_part[dash + 1..])) ?? return null;
    if (end_val < start_val) {
        return null;
    }

    let total_opt: u64? = null;
    if (!(total_part.len == 1 && total_part[0] == '*')) {
        const tot = parseU64Decimal(total_part) ?? return null;
        if (end_val >= tot) {
            return null;
        }
        total_opt = tot;
    }

    return ParsedContentRange {
        is_unsatisfied: false,
        start: start_val,
        end: end_val,
        total: total_opt,
    };
}
