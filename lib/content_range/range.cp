// Source: Absorbed from /data/dev/bun/src/http/SendFile.cp | Synced: 2026-11-22T11:12:13Z (RFC 9110 §14 HTTP Range / Content-Range)
// Pure-CP zero-allocation HTTP `Range` request parser and `Content-Range`
// response header parser/formatter for partial content (206 / 416).

pub struct ByteRange {
    /// Inclusive start byte offset.
    start: u64,
    /// Inclusive end byte offset (`end >= start`).
    end: u64,

    pub fn length(): u64 {
        return _.end - _.start + 1;
    }
}

pub enum RangeStatus: u8 {
    ok = 0,
    unsatisfiable = 1,
    invalid = 2,
}

pub struct ParsedRangeHeader {
    status: RangeStatus,
    ranges: [16]ByteRange,
    len: usize,

    pub fn first(): ByteRange? {
        if (_.status != .ok || _.len == 0) return null;
        return _.ranges[0];
    }
}

/// Parse an HTTP `Range: bytes=...` header against a resource of `total_size` bytes.
/// Supports `start-end`, open-ended `start-`, suffix `-suffix_len`, and comma-separated lists.
pub fn parseRangeHeader(header: []const u8, total_size: u64): ParsedRangeHeader {
    let res = ParsedRangeHeader{
        status: .invalid,
        ranges: [ByteRange{ start: 0, end: 0 }; 16],
        len: 0,
    };

    const s = trimOws(header);
    if (s.len < 6 || !startsWithIgnoreCase(s, "bytes=")) return res;

    const spec_list = trimOws(s[6..]);
    if (spec_list.len == 0) return res;

    if (total_size == 0) {
        res.status = .unsatisfiable;
        return res;
    }

    let pos: usize = 0;
    let saw_valid_syntax = false;
    while (pos < spec_list.len) {
        const start_p = pos;
        while (pos < spec_list.len && spec_list[pos] != ',') : (pos += 1) {}
        const item = trimOws(spec_list[start_p..pos]);
        if (pos < spec_list.len && spec_list[pos] == ',') pos += 1;

        if (item.len == 0) return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };

        const dash = findChar(item, '-') ?? return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
        const left = trimOws(item[0..dash]);
        const right = trimOws(item[dash + 1 ..]);
        if (left.len == 0 && right.len == 0) {
            return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
        }

        saw_valid_syntax = true;
        let r_start: u64 = 0;
        let r_end: u64 = 0;

        if (left.len == 0) {
            // Suffix range: `-N` (last N bytes)
            const suffix = parseU64(right) ?? return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
            if (suffix == 0) continue; // unsatisfiable per RFC 9110 §14.1.2
            if (suffix >= total_size) {
                r_start = 0;
                r_end = total_size - 1;
            } else {
                r_start = total_size - suffix;
                r_end = total_size - 1;
            }
        } else if (right.len == 0) {
            // Open-ended range: `N-`
            const st = parseU64(left) ?? return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
            if (st >= total_size) continue; // unsatisfiable
            r_start = st;
            r_end = total_size - 1;
        } else {
            // Closed range: `start-end`
            const st = parseU64(left) ?? return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
            const en = parseU64(right) ?? return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
            if (st > en) return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
            if (st >= total_size) continue; // unsatisfiable
            r_start = st;
            r_end = @min(en, total_size - 1);
        }

        if (res.len >= 16) return ParsedRangeHeader{ status: .invalid, ranges: res.ranges, len: 0 };
        res.ranges[res.len] = ByteRange{ start: r_start, end: r_end };
        res.len += 1;
    }

    if (!saw_valid_syntax) return res;
    if (res.len == 0) {
        res.status = .unsatisfiable;
        return res;
    }
    res.status = .ok;
    return res;
}

/// Format an HTTP 206 `Content-Range: bytes <start>-<end>/<total>` header into `out`.
pub fn formatContentRange(range: ByteRange, total_size: u64, out: []u8): ([]const u8)? {
    let p: usize = 0;
    if (!appendStr(out, &p, "bytes ")) return null;
    if (!appendU64(out, &p, range.start)) return null;
    if (!appendStr(out, &p, "-")) return null;
    if (!appendU64(out, &p, range.end)) return null;
    if (!appendStr(out, &p, "/")) return null;
    if (!appendU64(out, &p, total_size)) return null;
    return out[0..p];
}

/// Format an HTTP 416 `Content-Range: bytes */<total>` header into `out`.
pub fn formatUnsatisfiedRange(total_size: u64, out: []u8): ([]const u8)? {
    let p: usize = 0;
    if (!appendStr(out, &p, "bytes */")) return null;
    if (!appendU64(out, &p, total_size)) return null;
    return out[0..p];
}

pub struct ParsedContentRange {
    has_range: bool,
    range: ByteRange,
    has_total: bool,
    total_size: u64,
}

/// Parse an HTTP `Content-Range: bytes <start>-<end>/<total|*>` or `bytes */<total>` header.
pub fn parseContentRange(header: []const u8): ParsedContentRange? {
    const s = trimOws(header);
    if (s.len < 6 || !startsWithIgnoreCase(s, "bytes ")) return null;
    const rest = trimOws(s[6..]);
    const slash = findChar(rest, '/') ?? return null;
    const left = trimOws(rest[0..slash]);
    const right = trimOws(rest[slash + 1 ..]);
    if (left.len == 0 || right.len == 0) return null;

    let has_total = false;
    let total_size: u64 = 0;
    if (!strEq(right, "*")) {
        total_size = parseU64(right) ?? return null;
        has_total = true;
    }

    if (strEq(left, "*")) {
        if (!has_total) return null; // `bytes */*` is invalid per RFC 9110 §14.4
        return ParsedContentRange{
            has_range: false,
            range: ByteRange{ start: 0, end: 0 },
            has_total: true,
            total_size: total_size,
        };
    }

    const dash = findChar(left, '-') ?? return null;
    const st = parseU64(trimOws(left[0..dash])) ?? return null;
    const en = parseU64(trimOws(left[dash + 1 ..])) ?? return null;
    if (st > en) return null;
    if (has_total && en >= total_size) return null;

    return ParsedContentRange{
        has_range: true,
        range: ByteRange{ start: st, end: en },
        has_total: has_total,
        total_size: total_size,
    };
}

// ---------- Helpers ----------

fn findChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn parseU64(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u64, s[i] - '0');
    }
    return acc;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn appendStr(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.*] = s[i];
        pos.* += 1;
    }
    return true;
}

fn appendU64(out: []u8, pos: *usize, v_in: u64): bool {
    if (v_in == 0) return appendStr(out, pos, "0");
    let buf: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + @as(u8, @intCast(v % 10)));
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    while (n > 0) {
        n -= 1;
        out[pos.*] = buf[n];
        pos.* += 1;
    }
    return true;
}
