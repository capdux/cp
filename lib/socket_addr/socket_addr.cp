// Source: Absorbed from /data/dev/bun/src/http/http.cp (Socket Address host:port / [ipv6]:port Parser & Formatter)
// Pure-CP zero-allocation socket address parser and formatter supporting IPv4, bracketed IPv6, and hostnames.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn parsePort(s: []const u8, allow_zero: bool): u16? {
    if (s.len == 0 || s.len > 5) return null;
    if (s.len > 1 && s[0] == '0') return null; // reject leading zeros like :080
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u32, c - '0');
        if (val > 65535) return null;
        i += 1;
    }
    if (!allow_zero && val == 0) return null;
    return @as(u16, val);
}

fn isIpv4Literal(s: []const u8): bool {
    if (s.len < 7 || s.len > 15) return false;
    let parts: usize = 0;
    let i: usize = 0;
    while (parts < 4) {
        if (i >= s.len) return false;
        const start = i;
        let val: u32 = 0;
        while (i < s.len && s[i] != '.') {
            if (s[i] < '0' || s[i] > '9') return false;
            val = val * 10 + @as(u32, s[i] - '0');
            if (val > 255) return false;
            i += 1;
        }
        const d = i - start;
        if (d == 0 || d > 3) return false;
        if (d > 1 && s[start] == '0') return false;
        if (parts < 3) {
            if (i >= s.len || s[i] != '.') return false;
            i += 1;
        }
        parts += 1;
    }
    return i == s.len;
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

export enum HostKind {
    ipv4,
    ipv6,
    hostname,
}

export struct SocketAddr {
    host: []const u8, // unbracketed host (e.g. "::1", "127.0.0.1", "api.example.com")
    port: u16,
    kind: HostKind,

    /// Parse a strict `host:port` or `[ipv6]:port` endpoint (allows port 0 when `allow_port_zero == true`, e.g. bind `:0`).
    /// Also accepts Go-style `":8080"` as `"0.0.0.0"` + port `8080` when `allow_port_zero == true` (bind mode).
    pub fn parseWithOptions(raw: []const u8, allow_port_zero: bool, allow_empty_host: bool): SocketAddr? {
        if (raw.len == 0) return null;
        let wi: usize = 0;
        while (wi < raw.len) {
            if (isWs(raw[wi]) || raw[wi] == '/' || raw[wi] == '@') return null;
            wi += 1;
        }

        if (raw[0] == '[') {
            let cb: usize? = null;
            let k: usize = 1;
            while (k < raw.len) {
                if (raw[k] == ']') {
                    cb = k;
                    break;
                }
                k += 1;
            }
            const close_idx = cb ?? return null;
            const inner = raw[1..close_idx];
            if (inner.len < 2) return null;
            if (close_idx + 2 >= raw.len || raw[close_idx + 1] != ':') return null;
            const port = parsePort(raw[close_idx + 2..], allow_port_zero) ?? return null;
            return SocketAddr { host: inner, port: port, kind: HostKind.ipv6 };
        }

        let colon: usize? = null;
        let k: usize = 0;
        while (k < raw.len) {
            if (raw[k] == ':') {
                if (colon != null) return null; // unbracketed IPv6 ambiguous
                colon = k;
            }
            k += 1;
        }
        const cp = colon ?? return null;
        const host_part = raw[0..cp];
        const port = parsePort(raw[cp + 1..], allow_port_zero) ?? return null;

        if (host_part.len == 0) {
            if (!allow_empty_host) return null;
            return SocketAddr { host: "0.0.0.0", port: port, kind: HostKind.ipv4 };
        }

        const kind = if (isIpv4Literal(host_part)) HostKind.ipv4 else HostKind.hostname;
        return SocketAddr { host: host_part, port: port, kind: kind };
    }

    /// Parse a strict remote endpoint `host:port` or `[ipv6]:port` (requires non-empty host and port 1..=65535).
    pub fn parse(raw: []const u8): SocketAddr? {
        return SocketAddr.parseWithOptions(raw, false, false);
    }

    /// Parse a server bind address (allows `:8080` -> `0.0.0.0:8080` and ephemeral port `:0`).
    pub fn parseBind(raw: []const u8): SocketAddr? {
        return SocketAddr.parseWithOptions(raw, true, true);
    }

    /// Format canonically (`[ipv6]:port` for IPv6, `host:port` for IPv4/hostname).
    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (_.kind == HostKind.ipv6) {
            if (!writeSlice(out, &pos, "[")) return null;
            if (!writeSlice(out, &pos, _.host)) return null;
            if (!writeSlice(out, &pos, "]:")) return null;
        } else {
            if (!writeSlice(out, &pos, _.host)) return null;
            if (!writeSlice(out, &pos, ":")) return null;
        }
        if (!writeU16(out, &pos, _.port)) return null;
        return out[0..pos];
    }
}
