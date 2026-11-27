// packages/@cap/redis-url/src/index.cp
// Zero-allocation Redis / Valkey connection URL parser & redacting formatter.
// MIT clean-room pure CP implementation.

import { eqIgnoreCase, findByte, rfindByte } from "cap:ascii";
import { parseU16, writeStr, writeU64 } from "cap:fmt";

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

fn ruStartsWithIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn ruParseStrictU16(s: string): u16? {
    if (s.len == 0 || s[0] == ('+' as u8)) {
        return null;
    }
    return parseU16(s);
}

fn ruWrite(out: []u8, pos: *usize, src: string): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    pos.* = writeStr(src, out, pos.*);
    return true;
}

fn ruWriteU16(out: []u8, pos: *usize, val: u16): bool {
    let tmp: [8]u8 = undefined;
    const n = writeU64(val as u64, tmp[0..8], 0);
    return ruWrite(out, pos, tmp[0..n]);
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
                const dbOpt = ruParseStrictU16(dbStr);
                if (dbOpt == null) {
                    return res;
                }
                res.db = dbOpt ?? (0 as u16);
            }
        }

        // Split optional userinfo'@'hostport using the last '@'.
        const lastAtOpt = rfindByte(authority, '@' as u8);
        let hostport = authority;
        if (lastAtOpt != null) {
            const lastAt = lastAtOpt ?? (0 as usize);
            const uinfo = authority[0..lastAt];
            hostport = authority[(lastAt + 1)..authority.len];
            const colon = findByte(uinfo, ':' as u8, 0) ?? uinfo.len;
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
            const rbr = findByte(hostport, ']' as u8, 1) ?? hostport.len;
            if (rbr >= hostport.len || rbr == 1) {
                return res;
            }
            res.host = hostport[1..rbr];
            if (rbr + 1 < hostport.len) {
                if (hostport[rbr + 1] != (':' as u8)) {
                    return res;
                }
                const pStr = hostport[(rbr + 2)..hostport.len];
                const pOpt = ruParseStrictU16(pStr);
                if (pOpt == null || (pOpt ?? (0 as u16)) == (0 as u16)) {
                    return res;
                }
                res.port = pOpt ?? (0 as u16);
            }
        } else {
            const colon = findByte(hostport, ':' as u8, 0) ?? hostport.len;
            if (colon < hostport.len) {
                if (colon == 0) {
                    return res;
                }
                res.host = hostport[0..colon];
                const pStr = hostport[(colon + 1)..hostport.len];
                const pOpt = ruParseStrictU16(pStr);
                if (pOpt == null || (pOpt ?? (0 as u16)) == (0 as u16)) {
                    return res;
                }
                res.port = pOpt ?? (0 as u16);
            } else {
                res.host = hostport;
            }
        }
    }

    // Parse query parameters (supports `?db=N` override).
    if (queryPart.len > 0) {
        let qpos: usize = 0;
        while (qpos < queryPart.len) {
            const amp = findByte(queryPart, '&' as u8, qpos) ?? queryPart.len;
            const pair = queryPart[qpos..amp];
            if (amp < queryPart.len) {
                qpos = amp + 1;
            } else {
                qpos = queryPart.len;
            }

            const eq = findByte(pair, '=' as u8, 0) ?? pair.len;
            if (eq < pair.len) {
                const k = pair[0..eq];
                const v = pair[(eq + 1)..pair.len];
                if (eqIgnoreCase(k, "db")) {
                    const dbOpt = ruParseStrictU16(v);
                    if (dbOpt == null) {
                        return res;
                    }
                    res.db = dbOpt ?? (0 as u16);
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

    const isV6 = findByte(cfg.host, ':' as u8, 0) != null;
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
