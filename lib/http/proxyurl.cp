// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (HTTP / SOCKS Proxy URL Parser & Env Selector)
// Pure-CP zero-allocation proxy URL parser (http/https/socks5/socks5h with credentials) and env proxy selector.

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
    if (val == 0) return null;
    return @as(u16, val);
}

export enum ProxyScheme {
    http,
    https,
    socks4,
    socks5,
    socks5h,
}

export fn parseProxyScheme(s: []const u8): ProxyScheme? {
    if (eqIgnoreCase(s, "http")) return ProxyScheme.http;
    if (eqIgnoreCase(s, "https")) return ProxyScheme.https;
    if (eqIgnoreCase(s, "socks4")) return ProxyScheme.socks4;
    if (eqIgnoreCase(s, "socks5")) return ProxyScheme.socks5;
    if (eqIgnoreCase(s, "socks5h")) return ProxyScheme.socks5h;
    return null;
}

export fn defaultProxyPort(scheme: ProxyScheme): u16 {
    if (scheme == ProxyScheme.http) return 80;
    if (scheme == ProxyScheme.https) return 443;
    return 1080;
}

export struct ProxyUrl {
    scheme: ProxyScheme,
    username: ([]const u8)?,
    password: ([]const u8)?,
    host: []const u8,
    port: u16,

    /// Parse a proxy URL (`http://[user[:pass]@]host[:port][/]`, `socks5h://...`, or bare `host:port` defaulting to `http`).
    pub fn parse(raw: []const u8): ProxyUrl? {
        const s = trimWs(raw);
        if (s.len == 0) return null;

        let scheme = ProxyScheme.http;
        let rest = s;

        let sep: usize? = null;
        let i: usize = 0;
        while (i + 2 < s.len) {
            if (s[i] == ':' && s[i + 1] == '/' && s[i + 2] == '/') {
                sep = i;
                break;
            }
            i += 1;
        }
        if (sp := sep) {
            scheme = parseProxyScheme(s[0..sp]) ?? return null;
            rest = s[sp + 3..];
        }

        // Strip trailing path/query/fragment
        let auth_end: usize = 0;
        while (auth_end < rest.len && rest[auth_end] != '/' && rest[auth_end] != '?' && rest[auth_end] != '#') {
            auth_end += 1;
        }
        let auth = rest[0..auth_end];
        if (auth.len == 0) return null;

        // Optional user[:pass]@
        let username: ([]const u8)? = null;
        let password: ([]const u8)? = null;
        let at_pos: usize? = null;
        let ai: usize = 0;
        while (ai < auth.len) {
            if (auth[ai] == '@') at_pos = ai;
            ai += 1;
        }
        if (ap := at_pos) {
            const userinfo = auth[0..ap];
            auth = auth[ap + 1..];
            let colon: usize? = null;
            let ci: usize = 0;
            while (ci < userinfo.len) {
                if (userinfo[ci] == ':') {
                    colon = ci;
                    break;
                }
                ci += 1;
            }
            if (cp := colon) {
                username = userinfo[0..cp];
                password = userinfo[cp + 1..];
            } else if (userinfo.len > 0) {
                username = userinfo;
            }
        }
        if (auth.len == 0) return null;

        // Host and optional :port (IPv6 bracket aware)
        let host = auth;
        let port = defaultProxyPort(scheme);
        if (auth[0] == '[') {
            let cb: usize? = null;
            let k: usize = 1;
            while (k < auth.len) {
                if (auth[k] == ']') {
                    cb = k;
                    break;
                }
                k += 1;
            }
            const close_idx = cb ?? return null;
            host = auth[0..close_idx + 1];
            if (close_idx + 1 < auth.len) {
                if (auth[close_idx + 1] != ':') return null;
                port = parsePortStrict(auth[close_idx + 2..]) ?? return null;
            }
        } else {
            let colon_pos: usize? = null;
            let k: usize = 0;
            while (k < auth.len) {
                if (auth[k] == ':') {
                    if (colon_pos != null) return null;
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

        return ProxyUrl {
            scheme: scheme,
            username: username,
            password: password,
            host: host,
            port: port,
        };
    }

    pub fn isSocks(): bool {
        return _.scheme == ProxyScheme.socks4 || _.scheme == ProxyScheme.socks5 || _.scheme == ProxyScheme.socks5h;
    }

    pub fn resolvesDnsRemotely(): bool {
        return _.scheme == ProxyScheme.http || _.scheme == ProxyScheme.https || _.scheme == ProxyScheme.socks5h;
    }
}

export struct ProxyEnv {
    http_proxy: ([]const u8)?,
    https_proxy: ([]const u8)?,
    all_proxy: ([]const u8)?,
    no_proxy: ([]const u8)?,

    pub fn empty(): ProxyEnv {
        return ProxyEnv {
            http_proxy: null,
            https_proxy: null,
            all_proxy: null,
            no_proxy: null,
        };
    }
}

fn hostMatchesNoProxy(no_proxy: []const u8, target_host: []const u8): bool {
    let pos: usize = 0;
    while (pos < no_proxy.len) {
        const start = pos;
        while (pos < no_proxy.len && no_proxy[pos] != ',') pos += 1;
        const end = pos;
        if (pos < no_proxy.len && no_proxy[pos] == ',') pos += 1;
        let rule = trimWs(no_proxy[start..end]);
        if (rule.len == 0) continue;
        if (rule.len == 1 && rule[0] == '*') return true;
        if (rule[0] == '.') rule = rule[1..];
        if (rule.len == 0) continue;
        if (eqIgnoreCase(target_host, rule)) return true;
        if (target_host.len > rule.len && target_host[target_host.len - rule.len - 1] == '.') {
            if (eqIgnoreCase(target_host[target_host.len - rule.len..], rule)) return true;
        }
    }
    return false;
}

/// Select the effective `ProxyUrl` for a target `(target_scheme, target_host)` from `ProxyEnv`.
/// Returns null if bypassed by `no_proxy` or no proxy is configured.
export fn selectProxy(env: ProxyEnv, target_scheme: []const u8, target_host: []const u8): ProxyUrl? {
    if (np := env.no_proxy) {
        if (hostMatchesNoProxy(np, target_host)) return null;
    }
    if (eqIgnoreCase(target_scheme, "https") || eqIgnoreCase(target_scheme, "wss")) {
        if (hp := env.https_proxy) {
            if (hp.len > 0) return ProxyUrl.parse(hp);
        }
    } else if (eqIgnoreCase(target_scheme, "http") || eqIgnoreCase(target_scheme, "ws")) {
        if (hp := env.http_proxy) {
            if (hp.len > 0) return ProxyUrl.parse(hp);
        }
    }
    if (ap := env.all_proxy) {
        if (ap.len > 0) return ProxyUrl.parse(ap);
    }
    return null;
}
