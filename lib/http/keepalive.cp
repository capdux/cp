// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (HTTP Keep-Alive Header & Connection Persistence Policy)
// Pure-CP zero-allocation Keep-Alive header parser/formatter and RFC 9112 §9.3 connection persistence evaluator.

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

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) return writeSlice(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

export struct KeepAliveParams {
    timeout_sec: u64?,
    max_requests: u64?,

    /// Parse `Keep-Alive: timeout=5, max=100` (case-insensitive, skips unknown parameters).
    pub fn parse(header: []const u8): KeepAliveParams {
        let res = KeepAliveParams {
            timeout_sec: null,
            max_requests: null,
        };
        let pos: usize = 0;
        while (pos < header.len) {
            const start = pos;
            while (pos < header.len && header[pos] != ',') pos += 1;
            const end = pos;
            if (pos < header.len && header[pos] == ',') pos += 1;

            const seg = trimWs(header[start..end]);
            if (seg.len == 0) continue;
            let eq: usize? = null;
            let k: usize = 0;
            while (k < seg.len) {
                if (seg[k] == '=') {
                    eq = k;
                    break;
                }
                k += 1;
            }
            if (eq == null) continue;
            const ep = eq.?;
            const key = trimWs(seg[0..ep]);
            let val = trimWs(seg[ep + 1..]);
            if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                val = val[1..val.len - 1];
            }
            if (eqIgnoreCase(key, "timeout") && res.timeout_sec == null) {
                res.timeout_sec = parseU64Strict(val);
            } else if (eqIgnoreCase(key, "max") && res.max_requests == null) {
                res.max_requests = parseU64Strict(val);
            }
        }
        return res;
    }

    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        let first = true;
        if (t := _.timeout_sec) {
            if (!writeSlice(out, &pos, "timeout=")) return null;
            if (!writeU64(out, &pos, t)) return null;
            first = false;
        }
        if (m := _.max_requests) {
            if (!first) {
                if (!writeSlice(out, &pos, ", ")) return null;
            }
            if (!writeSlice(out, &pos, "max=")) return null;
            if (!writeU64(out, &pos, m)) return null;
        }
        return out[0..pos];
    }
}

fn connectionContains(header: []const u8, token: []const u8): bool {
    let pos: usize = 0;
    while (pos < header.len) {
        const start = pos;
        while (pos < header.len && header[pos] != ',') pos += 1;
        const end = pos;
        if (pos < header.len && header[pos] == ',') pos += 1;
        if (eqIgnoreCase(trimWs(header[start..end]), token)) return true;
    }
    return false;
}

/// RFC 9112 §9.3 connection persistence decision:
/// - If `Connection` contains `close` -> false.
/// - Else if HTTP/1.1 (`minor_version >= 1`) -> true by default.
/// - Else (HTTP/1.0) -> true only if `Connection` explicitly contains `keep-alive`.
export fn shouldKeepAlive(http_minor_version: u8, connection_header: ([]const u8)?): bool {
    if (conn := connection_header) {
        if (connectionContains(conn, "close")) return false;
        if (http_minor_version >= 1) return true;
        return connectionContains(conn, "keep-alive");
    }
    return http_minor_version >= 1;
}
