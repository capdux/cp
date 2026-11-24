// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T16:49:05Z (WHATWG Origin Parser, Same-Origin & Same-Site Comparator)
// Pure-CP zero-allocation WHATWG Origin tuple parser, default-port normalization, same-origin/same-site checks, and formatter.

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

fn defaultPort(scheme: []const u8): u16? {
    if (eqIgnoreCase(scheme, "http") || eqIgnoreCase(scheme, "ws")) return 80;
    if (eqIgnoreCase(scheme, "https") || eqIgnoreCase(scheme, "wss")) return 443;
    if (eqIgnoreCase(scheme, "ftp")) return 21;
    return null;
}

fn parsePortStrict(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u32, c - '0');
        if (val > 65535) return null;
        i += 1;
    }
    return @as(u16, val);
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

fn writeLowerSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = toLowerAscii(s[i]);
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeU16(out: []u8, pos: *usize, val: u16): bool {
    if (val == 0) return writeSlice(out, pos, "0");
    let tmp: [8]u8 = [0; 8];
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

export struct Origin {
    scheme: []const u8,
    host: []const u8,
    port: u16?,

    /// Parse a WHATWG tuple origin from a URL or `Origin` header (e.g. `"https://Example.COM:443/path"`).
    /// Returns null for opaque origins (`"null"`, `data:`, `file:`, missing authority, or invalid port).
    pub static fn parse(raw: []const u8): Origin? {
        // Find "://"
        let sep: usize? = null;
        let i: usize = 0;
        while (i + 2 < raw.len) {
            if (raw[i] == ':' && raw[i + 1] == '/' && raw[i + 2] == '/') {
                sep = i;
                break;
            }
            i += 1;
        }
        const sp = sep ?? return null;
        const scheme = raw[0..sp];
        if (scheme.len == 0) return null;

        // Extract authority up to '/', '?', '#'
        let auth_start = sp + 3;
        let auth_end = auth_start;
        while (auth_end < raw.len && raw[auth_end] != '/' && raw[auth_end] != '?' && raw[auth_end] != '#') {
            auth_end += 1;
        }
        let auth = raw[auth_start..auth_end];
        if (auth.len == 0) return null;

        // Strip optional userinfo (`user:pass@`)
        let at_pos: usize? = null;
        let ai: usize = 0;
        while (ai < auth.len) {
            if (auth[ai] == '@') at_pos = ai;
            ai += 1;
        }
        if (ap := at_pos) {
            auth = auth[ap + 1..];
        }
        if (auth.len == 0) return null;

        // Parse host (IPv6 `[...]` or reg-name) and optional `:port`
        let host: []const u8 = auth;
        let port: u16? = null;

        if (auth[0] == '[') {
            let close_b: usize? = null;
            let k: usize = 1;
            while (k < auth.len) {
                if (auth[k] == ']') {
                    close_b = k;
                    break;
                }
                k += 1;
            }
            const cb = close_b ?? return null;
            host = auth[0..cb + 1];
            if (cb + 1 < auth.len) {
                if (auth[cb + 1] != ':') return null;
                port = parsePortStrict(auth[cb + 2..]) ?? return null;
            }
        } else {
            let colon_pos: usize? = null;
            let k: usize = 0;
            while (k < auth.len) {
                if (auth[k] == ':') {
                    if (colon_pos != null) return null; // multiple colons outside IPv6
                    colon_pos = k;
                }
                k += 1;
            }
            if (cp := colon_pos) {
                host = auth[0..cp];
                if (cp + 1 < auth.len) {
                    port = parsePortStrict(auth[cp + 1..]) ?? return null;
                }
            }
        }

        if (host.len == 0) return null;

        // Strip trailing dot on host if not bare "."
        if (host.len > 1 && host[host.len - 1] == '.') {
            host = host[0..host.len - 1];
        }

        // Normalize default port to effective port
        const eff_port = port ?? defaultPort(scheme);
        return Origin {
            scheme: scheme,
            host: host,
            port: eff_port,
        };
    }

    /// WHATWG Same-Origin comparison (case-insensitive scheme & host, equal effective port).
    pub fn sameOrigin(other: Origin): bool {
        if (!eqIgnoreCase(_.scheme, other.scheme)) return false;
        if (!eqIgnoreCase(_.host, other.host)) return false;
        if (_.port == null && other.port == null) return true;
        if (_.port == null || other.port == null) return false;
        return _.port.? == other.port.?;
    }

    /// True for `https:`, `wss:`, or loopback hosts (`localhost`, `127.0.0.1`, `[::1]`).
    pub fn isPotentiallyTrustworthy(): bool {
        if (eqIgnoreCase(_.scheme, "https") || eqIgnoreCase(_.scheme, "wss")) return true;
        if (eqIgnoreCase(_.host, "localhost") || eqIgnoreCase(_.host, "127.0.0.1") || eqIgnoreCase(_.host, "[::1]")) {
            return true;
        }
        // *.localhost is also trustworthy per W3C Secure Contexts
        if (_.host.len > 10 && eqIgnoreCase(_.host[_.host.len - 10..], ".localhost")) {
            return true;
        }
        return false;
    }

    /// Canonical serialized origin (`scheme://host[:non_default_port]`), lowercase.
    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!writeLowerSlice(out, &pos, _.scheme)) return null;
        if (!writeSlice(out, &pos, "://")) return null;
        if (!writeLowerSlice(out, &pos, _.host)) return null;
        if (p := _.port) {
            const def = defaultPort(_.scheme);
            if (def == null || def.? != p) {
                if (!writeSlice(out, &pos, ":")) return null;
                if (!writeU16(out, &pos, p)) return null;
            }
        }
        return out[0..pos];
    }
}

/// Convenience helper: check if two URLs/origins share the same WHATWG origin.
export fn isSameOriginUrl(a: []const u8, b: []const u8): bool {
    const oa = Origin.parse(a) ?? return false;
    const ob = Origin.parse(b) ?? return false;
    return oa.sameOrigin(ob);
}
