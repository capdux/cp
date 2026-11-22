// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (RFC 9110 §14.1.2 HTTP Range Request Parser)
// Pure-CP zero-allocation RFC 9110 Range header parser, multi-range iterator, and byte-range resolver.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = @as(u64, c - '0');
        if (val > 1844674407370955161) return null;
        if (val == 1844674407370955161 && d > 5) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

export enum RangeSpecKind {
    bounded,   // start..=end
    open_end,  // start..
    suffix,    // last N bytes (-suffix_len)
}

export struct ByteRangeSpec {
    kind: RangeSpecKind,
    start: u64,
    end: u64,

    /// Resolve this spec against a concrete `entity_len` into an inclusive `[start, end]` byte range.
    /// Returns null if unsatisfiable (HTTP 416) or `entity_len == 0`.
    pub fn resolve(entity_len: u64): ResolvedByteRange? {
        if (entity_len == 0) return null;
        if (_.kind == RangeSpecKind.bounded) {
            if (_.start > _.end || _.start >= entity_len) return null;
            let clamped_end = _.end;
            if (clamped_end >= entity_len) clamped_end = entity_len - 1;
            return ResolvedByteRange {
                start: _.start,
                end: clamped_end,
                total_len: entity_len,
            };
        }
        if (_.kind == RangeSpecKind.open_end) {
            if (_.start >= entity_len) return null;
            return ResolvedByteRange {
                start: _.start,
                end: entity_len - 1,
                total_len: entity_len,
            };
        }
        // suffix: last `end` bytes
        const suffix_len = _.end;
        if (suffix_len == 0) return null;
        if (suffix_len >= entity_len) {
            return ResolvedByteRange {
                start: 0,
                end: entity_len - 1,
                total_len: entity_len,
            };
        }
        return ResolvedByteRange {
            start: entity_len - suffix_len,
            end: entity_len - 1,
            total_len: entity_len,
        };
    }
}

export struct ResolvedByteRange {
    start: u64,
    end: u64,
    total_len: u64,

    pub fn length(): u64 {
        return (_.end - _.start) + 1;
    }
}

export struct ByteRangeIter {
    rest: []const u8,
    pos: usize,
    had_error: bool,

    /// Initialize iterator from a `Range` header value (must start with `bytes=`, case-insensitive).
    pub fn init(header: []const u8): ByteRangeIter? {
        const s = trimWs(header);
        if (s.len < 7) return null;
        let eq_pos: usize? = null;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '=') {
                eq_pos = i;
                break;
            }
            i += 1;
        }
        const ep = eq_pos ?? return null;
        const unit = trimWs(s[0..ep]);
        if (!eqIgnoreCase(unit, "bytes")) return null;
        const specs = trimWs(s[ep + 1..]);
        if (specs.len == 0) return null;
        return ByteRangeIter { rest: specs, pos: 0, had_error: false };
    }

    pub fn next(*_): ByteRangeSpec? {
        if (_.had_error) return null;
        while (_.pos < _.rest.len) {
            const start = _.pos;
            while (_.pos < _.rest.len && _.rest[_.pos] != ',') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.rest.len && _.rest[_.pos] == ',') {
                _.pos += 1;
            }
            const item = trimWs(_.rest[start..end]);
            if (item.len == 0) {
                _.had_error = true;
                return null;
            }
            let dash_pos: usize? = null;
            let k: usize = 0;
            while (k < item.len) {
                if (item[k] == '-') {
                    dash_pos = k;
                    break;
                }
                k += 1;
            }
            if (dash_pos == null) {
                _.had_error = true;
                return null;
            }
            const dp = dash_pos.?;
            const left = trimWs(item[0..dp]);
            const right = trimWs(item[dp + 1..]);

            if (left.len == 0 && right.len == 0) {
                _.had_error = true;
                return null;
            }
            if (left.len == 0) {
                const slen_opt = parseU64Strict(right);
                if (slen_opt == null) {
                    _.had_error = true;
                    return null;
                }
                return ByteRangeSpec { kind: RangeSpecKind.suffix, start: 0, end: slen_opt.? };
            }
            const s_opt = parseU64Strict(left);
            if (s_opt == null) {
                _.had_error = true;
                return null;
            }
            const s_val = s_opt.?;
            if (right.len == 0) {
                return ByteRangeSpec { kind: RangeSpecKind.open_end, start: s_val, end: 0 };
            }
            const e_opt = parseU64Strict(right);
            if (e_opt == null) {
                _.had_error = true;
                return null;
            }
            const e_val = e_opt.?;
            if (e_val < s_val) {
                _.had_error = true;
                return null;
            }
            return ByteRangeSpec { kind: RangeSpecKind.bounded, start: s_val, end: e_val };
        }
        return null;
    }

    pub fn hasError(): bool {
        return _.had_error;
    }
}

/// Convenience helper: resolve a single-range `Range: bytes=...` header against `entity_len`.
/// Returns null if malformed, multi-range, or unsatisfiable.
export fn resolveSingleByteRange(header: []const u8, entity_len: u64): ResolvedByteRange? {
    let it = ByteRangeIter.init(header) ?? return null;
    const first = it.next() ?? return null;
    if (it.hasError()) return null;
    if (it.next() != null || it.hasError()) return null;
    return first.resolve(entity_len);
}
