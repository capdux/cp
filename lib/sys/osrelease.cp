// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T15:22:05Z (freedesktop.org /etc/os-release Parser)
// Pure-CP zero-allocation freedesktop.org /etc/os-release parser with shell quote unescaping and ID_LIKE matching.

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

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

export struct OsReleaseField {
    key: []const u8,
    raw_value: []const u8,

    /// Unquote and unescape the value into `out` per freedesktop.org os-release spec.
    pub fn decodeValue(out: []u8): ([]const u8)? {
        const s = _.raw_value;
        if (s.len >= 2 && s[0] == '\'' && s[s.len - 1] == '\'') {
            const inner = s[1..s.len - 1];
            if (inner.len > out.len) return null;
            let i: usize = 0;
            while (i < inner.len) {
                out[i] = inner[i];
                i += 1;
            }
            return out[0..inner.len];
        }
        if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
            const inner = s[1..s.len - 1];
            let pos: usize = 0;
            let i: usize = 0;
            while (i < inner.len) {
                if (inner[i] == '\\' && i + 1 < inner.len) {
                    const nxt = inner[i + 1];
                    if (nxt == '"' || nxt == '\\' || nxt == '$' || nxt == '`') {
                        if (pos >= out.len) return null;
                        out[pos] = nxt;
                        pos += 1;
                        i += 2;
                        continue;
                    }
                }
                if (pos >= out.len) return null;
                out[pos] = inner[i];
                pos += 1;
                i += 1;
            }
            return out[0..pos];
        }
        if (s.len > out.len) return null;
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i += 1;
        }
        return out[0..s.len];
    }
}

export struct OsReleaseIter {
    src: []const u8,
    pos: usize,

    pub static fn init(content: []const u8): OsReleaseIter {
        return OsReleaseIter { src: content, pos: 0 };
    }

    pub fn next(*_): OsReleaseField? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') _.pos += 1;
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') _.pos += 1;

            const line = trimWs(_.src[start..end]);
            if (line.len == 0 || line[0] == '#') continue;

            let eq: usize? = null;
            let k: usize = 0;
            while (k < line.len) {
                if (line[k] == '=') {
                    eq = k;
                    break;
                }
                k += 1;
            }
            if (eq == null) continue;
            const ep = eq.?;
            const key = trimWs(line[0..ep]);
            if (key.len == 0) continue;
            const val = trimWs(line[ep + 1..]);
            return OsReleaseField { key: key, raw_value: val };
        }
        return null;
    }
}

/// Lookup a key in `/etc/os-release` (later assignment overrides earlier per spec) and decode into `out`.
export fn getOsReleaseField(content: []const u8, key: []const u8, out: []u8): ([]const u8)? {
    let found: OsReleaseField? = null;
    let it = OsReleaseIter.init(content);
    while (f := it.next()) {
        if (strEq(f.key, key)) {
            found = f;
        }
    }
    if (f := found) {
        return f.decodeValue(out);
    }
    return null;
}

/// Check if an `/etc/os-release` document matches `distro_id` via `ID` or whitespace-separated `ID_LIKE`.
export fn isOsLike(content: []const u8, distro_id: []const u8): bool {
    if (distro_id.len == 0) return false;
    let buf: [256]u8 = [0; 256];
    if (id := getOsReleaseField(content, "ID", buf[0..])) {
        if (strEq(id, distro_id)) return true;
    }
    if (id_like := getOsReleaseField(content, "ID_LIKE", buf[0..])) {
        let pos: usize = 0;
        while (pos < id_like.len) {
            while (pos < id_like.len && isWs(id_like[pos])) pos += 1;
            const start = pos;
            while (pos < id_like.len && !isWs(id_like[pos])) pos += 1;
            if (pos > start && strEq(id_like[start..pos], distro_id)) return true;
        }
    }
    return false;
}
