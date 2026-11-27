// packages/@cap/redis-url/src/index.cp
// Zero-allocation Redis / Valkey connection URL parser & redacting formatter.
// MIT clean-room pure CP implementation.

/// Zero-copy parsed Redis / Valkey connection configuration.
/// Defaults: `port = 6379`, `db = 0`.
pub struct RedisConfig {
    pub tls: bool,
    pub is_unix: bool,
    pub host: string,
    pub port: u16,
    pub username: string,
    pub password: string,
    pub db: u16,
    pub socket_path: string,
    pub valid: bool,
}

fn ruToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn ruEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (ruToLower(a[i]) != ruToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn ruStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return ruEqIgnoreCase(s[0..prefix.len], prefix);
}

fn ruContainsByte(s: []const u8, target: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn ruParseU16(s: []const u8, out: *u16): bool {
    if (s.len == 0 || s.len > 5) {
        return false;
    }
    let acc: u32 = 0 as u32;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return false;
        }
        acc = acc * (10 as u32) + ((c - ('0' as u8)) as u32);
        if (acc > (65535 as u32)) {
            return false;
        }
        i += 1;
    }
    out.* = acc as u16;
    return true;
}

fn ruWrite(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn ruWriteU16(out: []u8, pos: *usize, val: u16): bool {
    if (val == (0 as u16)) {
        return ruWrite(out, pos, "0");
    }
    let rev: [8]u8 = undefined;
    let n: usize = 0;
    let cur: u32 = val as u32;
    while (cur > (0 as u32)) {
        rev[n] = ('0' as u8) + ((cur % (10 as u32)) as u8);
        n += 1;
        cur = cur / (10 as u32);
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.*] = rev[n - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// Parse a Redis / Valkey connection URL:
/// - Schemes: `redis://`, `rediss://` (`tls=true`), `redis+unix://`, `unix://`.
/// - Auth: none, password-only (`redis://:secret@host`), or ACL (`redis://alice:secret@host:6380/3`).
/// - Hosts: IPv4, hostname, or bracketed IPv6 (`[::1]:6379`).
/// - DB index: `/3` path or `?db=5` query parameter override.
pub fn parseRedisUrl(url: string): RedisConfig {
    let res = RedisConfig{
        tls: false,
        is_unix: false,
        host: "",
        port: 6379 as u16,
        username: "",
        password: "",
        db: 0 as u16,
        socket_path: "",
        valid: false,
    };

    let afterScheme: usize = 0;
    if (ruStartsWithIgnoreCase(url, "redis://")) {
        afterScheme = 8;
    } else if (ruStartsWithIgnoreCase(url, "rediss://")) {
        res.tls = true;
        afterScheme = 9;
    } else if (ruStartsWithIgnoreCase(url, "redis+unix://")) {
        res.is_unix = true;
        res.port = 0 as u16;
        afterScheme = 13;
    } else if (ruStartsWithIgnoreCase(url, "unix://")) {
        res.is_unix = true;
        res.port = 0 as u16;
        afterScheme = 7;
    } else {
        return res;
    }

    const body = url[afterScheme..url.len];
    if (body.len == 0) {
        return res;
    }

    // Split optional '?query'.
    let qmark = body.len;
    let qi: usize = 0;
    while (qi < body.len) {
        if (body[qi] == ('?' as u8)) {
            qmark = qi;
            break;
        }
        qi += 1;
    }

    const mainPart = body[0..qmark];
    let queryPart: []const u8 = "";
    if (qmark < body.len) {
        queryPart = body[(qmark + 1)..body.len];
    }

    if (res.is_unix) {
        if (mainPart.len == 0) {
            return res;
        }
        // Optional userinfo before first '/' (e.g. redis+unix://:pass@/tmp/redis.sock).
        let sockSlice = mainPart;
        if (mainPart[0] != ('/' as u8)) {
            let atIdx = mainPart.len;
            let ai: usize = 0;
            while (ai < mainPart.len && mainPart[ai] != ('/' as u8)) {
                if (mainPart[ai] == ('@' as u8)) {
                    atIdx = ai;
                    break;
                }
                ai += 1;
            }
            if (atIdx < mainPart.len) {
                const uinfo = mainPart[0..atIdx];
                let colon = uinfo.len;
                let ci: usize = 0;
                while (ci < uinfo.len) {
                    if (uinfo[ci] == (':' as u8)) {
                        colon = ci;
                        break;
                    }
                    ci += 1;
                }
                if (colon < uinfo.len) {
                    res.username = uinfo[0..colon];
                    res.password = uinfo[(colon + 1)..uinfo.len];
                } else {
                    res.password = uinfo;
                }
                sockSlice = mainPart[(atIdx + 1)..mainPart.len];
            }
        }
        if (sockSlice.len == 0 || sockSlice[0] != ('/' as u8)) {
            return res;
        }
        res.socket_path = sockSlice;
    } else {
        // TCP / TLS: split authority and optional '/db'.
        let slash = mainPart.len;
        let si: usize = 0;
        while (si < mainPart.len) {
            if (mainPart[si] == ('/' as u8)) {
                slash = si;
                break;
            }
            si += 1;
        }

        const authority = mainPart[0..slash];
        if (authority.len == 0) {
            return res;
        }

        if (slash < mainPart.len) {
            const dbStr = mainPart[(slash + 1)..mainPart.len];
            if (dbStr.len > 0) {
                let dbVal: u16 = 0 as u16;
                if (!ruParseU16(dbStr, &dbVal)) {
                    return res;
                }
                res.db = dbVal;
            }
        }

        // Split optional userinfo'@'hostport using the last '@'.
        let hasAt = false;
        let lastAt: usize = 0;
        let ai: usize = 0;
        while (ai < authority.len) {
            if (authority[ai] == ('@' as u8)) {
                hasAt = true;
                lastAt = ai;
            }
            ai += 1;
        }

        let hostport = authority;
        if (hasAt) {
            const uinfo = authority[0..lastAt];
            hostport = authority[(lastAt + 1)..authority.len];
            let colon = uinfo.len;
            let ci: usize = 0;
            while (ci < uinfo.len) {
                if (uinfo[ci] == (':' as u8)) {
                    colon = ci;
                    break;
                }
                ci += 1;
            }
            if (colon < uinfo.len) {
                res.username = uinfo[0..colon];
                res.password = uinfo[(colon + 1)..uinfo.len];
            } else {
                res.password = uinfo;
            }
        }

        if (hostport.len == 0) {
            return res;
        }

        // Parse IPv6 [host]:port or host[:port].
        if (hostport[0] == ('[' as u8)) {
            let rbr: usize = 1;
            while (rbr < hostport.len && hostport[rbr] != (']' as u8)) {
                rbr += 1;
            }
            if (rbr >= hostport.len || rbr == 1) {
                return res;
            }
            res.host = hostport[1..rbr];
            if (rbr + 1 < hostport.len) {
                if (hostport[rbr + 1] != (':' as u8)) {
                    return res;
                }
                const pStr = hostport[(rbr + 2)..hostport.len];
                let pVal: u16 = 0 as u16;
                if (!ruParseU16(pStr, &pVal) || pVal == (0 as u16)) {
                    return res;
                }
                res.port = pVal;
            }
        } else {
            let colon = hostport.len;
            let ci: usize = 0;
            while (ci < hostport.len) {
                if (hostport[ci] == (':' as u8)) {
                    colon = ci;
                    break;
                }
                ci += 1;
            }
            if (colon < hostport.len) {
                if (colon == 0) {
                    return res;
                }
                res.host = hostport[0..colon];
                const pStr = hostport[(colon + 1)..hostport.len];
                let pVal: u16 = 0 as u16;
                if (!ruParseU16(pStr, &pVal) || pVal == (0 as u16)) {
                    return res;
                }
                res.port = pVal;
            } else {
                res.host = hostport;
            }
        }
    }

    // Parse query parameters (supports `?db=N` override).
    if (queryPart.len > 0) {
        let qpos: usize = 0;
        while (qpos < queryPart.len) {
            let amp = qpos;
            while (amp < queryPart.len && queryPart[amp] != ('&' as u8)) {
                amp += 1;
            }
            const pair = queryPart[qpos..amp];
            if (amp < queryPart.len) {
                qpos = amp + 1;
            } else {
                qpos = queryPart.len;
            }

            let eq: usize = 0;
            while (eq < pair.len && pair[eq] != ('=' as u8)) {
                eq += 1;
            }
            if (eq < pair.len) {
                const k = pair[0..eq];
                const v = pair[(eq + 1)..pair.len];
                if (ruEqIgnoreCase(k, "db")) {
                    let dbOverride: u16 = 0 as u16;
                    if (!ruParseU16(v, &dbOverride)) {
                        return res;
                    }
                    res.db = dbOverride;
                }
            }
        }
    }

    res.valid = true;
    return res;
}

/// Format a `RedisConfig` back into a standard Redis URL in `out`.
/// When `redact_password` is `true`, any non-empty password is replaced with `***` for safe logging.
/// Returns bytes written, or `0` on invalid config or insufficient `out` buffer.
pub fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize {
    if (!cfg.valid) {
        return 0;
    }

    let pos: usize = 0;

    if (cfg.is_unix) {
        if (cfg.socket_path.len == 0) {
            return 0;
        }
        if (!ruWrite(out, &pos, "redis+unix://")) {
            return 0;
        }
        if (cfg.username.len > 0 || cfg.password.len > 0) {
            if (!ruWrite(out, &pos, cfg.username)) {
                return 0;
            }
            if (!ruWrite(out, &pos, ":")) {
                return 0;
            }
            if (cfg.password.len > 0) {
                if (redact_password) {
                    if (!ruWrite(out, &pos, "***")) {
                        return 0;
                    }
                } else {
                    if (!ruWrite(out, &pos, cfg.password)) {
                        return 0;
                    }
                }
            }
            if (!ruWrite(out, &pos, "@")) {
                return 0;
            }
        }
        if (!ruWrite(out, &pos, cfg.socket_path)) {
            return 0;
        }
        if (cfg.db > (0 as u16)) {
            if (!ruWrite(out, &pos, "?db=")) {
                return 0;
            }
            if (!ruWriteU16(out, &pos, cfg.db)) {
                return 0;
            }
        }
        return pos;
    }

    if (cfg.host.len == 0) {
        return 0;
    }

    if (cfg.tls) {
        if (!ruWrite(out, &pos, "rediss://")) {
            return 0;
        }
    } else {
        if (!ruWrite(out, &pos, "redis://")) {
            return 0;
        }
    }

    if (cfg.username.len > 0 || cfg.password.len > 0) {
        if (!ruWrite(out, &pos, cfg.username)) {
            return 0;
        }
        if (!ruWrite(out, &pos, ":")) {
            return 0;
        }
        if (cfg.password.len > 0) {
            if (redact_password) {
                if (!ruWrite(out, &pos, "***")) {
                    return 0;
                }
            } else {
                if (!ruWrite(out, &pos, cfg.password)) {
                    return 0;
                }
            }
        }
        if (!ruWrite(out, &pos, "@")) {
            return 0;
        }
    }

    const isV6 = ruContainsByte(cfg.host, ':' as u8);
    if (isV6) {
        if (!ruWrite(out, &pos, "[")) {
            return 0;
        }
        if (!ruWrite(out, &pos, cfg.host)) {
            return 0;
        }
        if (!ruWrite(out, &pos, "]")) {
            return 0;
        }
    } else {
        if (!ruWrite(out, &pos, cfg.host)) {
            return 0;
        }
    }

    let port = cfg.port;
    if (port == (0 as u16)) {
        port = 6379 as u16;
    }
    if (!ruWrite(out, &pos, ":")) {
        return 0;
    }
    if (!ruWriteU16(out, &pos, port)) {
        return 0;
    }

    if (cfg.db > (0 as u16)) {
        if (!ruWrite(out, &pos, "/")) {
            return 0;
        }
        if (!ruWriteU16(out, &pos, cfg.db)) {
            return 0;
        }
    }

    return pos;
}

export {
    RedisConfig,
    parseRedisUrl,
    formatRedisUrl,
};
