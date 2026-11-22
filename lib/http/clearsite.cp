// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (W3C Clear-Site-Data Header Parser & Builder)
// Pure-CP zero-allocation W3C Clear-Site-Data response header parser and canonical builder.

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

fn appendToken(out: []u8, pos: *usize, first: *bool, tok: []const u8): bool {
    if (!first.*) {
        if (pos.* + 2 > out.len) return false;
        out[pos.*] = ',';
        out[pos.* + 1] = ' ';
        pos.* += 2;
    }
    first.* = false;
    if (pos.* + tok.len > out.len) return false;
    let i: usize = 0;
    while (i < tok.len) {
        out[pos.* + i] = tok[i];
        i += 1;
    }
    pos.* += tok.len;
    return true;
}

export struct ClearSiteData {
    cache: bool,
    cookies: bool,
    storage: bool,
    execution_contexts: bool,
    client_hints: bool,

    pub fn none(): ClearSiteData {
        return ClearSiteData {
            cache: false,
            cookies: false,
            storage: false,
            execution_contexts: false,
            client_hints: false,
        };
    }

    pub fn all(): ClearSiteData {
        return ClearSiteData {
            cache: true,
            cookies: true,
            storage: true,
            execution_contexts: true,
            client_hints: true,
        };
    }

    /// Parse W3C Clear-Site-Data header (`"cache", "cookies", "storage", "executionContexts"`, `"clientHints"`, or `"*"`).
    /// Per W3C Clear-Site-Data §3.1, directives are case-sensitive quoted strings; unknown or unquoted tokens are ignored.
    pub fn parse(header: []const u8): ClearSiteData {
        let res = ClearSiteData.none();
        let pos: usize = 0;
        while (pos < header.len) {
            const start = pos;
            while (pos < header.len && header[pos] != ',') pos += 1;
            const end = pos;
            if (pos < header.len && header[pos] == ',') pos += 1;
            const tok = trimWs(header[start..end]);
            if (tok.len < 3 || tok[0] != '"' || tok[tok.len - 1] != '"') continue;
            const inner = tok[1..tok.len - 1];
            if (strEq(inner, "*")) {
                return ClearSiteData.all();
            } else if (strEq(inner, "cache")) {
                res.cache = true;
            } else if (strEq(inner, "cookies")) {
                res.cookies = true;
            } else if (strEq(inner, "storage")) {
                res.storage = true;
            } else if (strEq(inner, "executionContexts")) {
                res.execution_contexts = true;
            } else if (strEq(inner, "clientHints")) {
                res.client_hints = true;
            }
        }
        return res;
    }

    pub fn isEmpty(): bool {
        return !_.cache && !_.cookies && !_.storage && !_.execution_contexts && !_.client_hints;
    }

    pub fn isAll(): bool {
        return _.cache && _.cookies && _.storage && _.execution_contexts && _.client_hints;
    }

    /// Format into canonical W3C `Clear-Site-Data` header value (`"*"` if all flags set).
    pub fn format(out: []u8): ([]const u8)? {
        if (_.isAll()) {
            if (out.len < 3) return null;
            out[0] = '"';
            out[1] = '*';
            out[2] = '"';
            return out[0..3];
        }
        let pos: usize = 0;
        let first = true;
        if (_.cache) {
            if (!appendToken(out, &pos, &first, "\"cache\"")) return null;
        }
        if (_.cookies) {
            if (!appendToken(out, &pos, &first, "\"cookies\"")) return null;
        }
        if (_.storage) {
            if (!appendToken(out, &pos, &first, "\"storage\"")) return null;
        }
        if (_.execution_contexts) {
            if (!appendToken(out, &pos, &first, "\"executionContexts\"")) return null;
        }
        if (_.client_hints) {
            if (!appendToken(out, &pos, &first, "\"clientHints\"")) return null;
        }
        return out[0..pos];
    }
}
