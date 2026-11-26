// packages/@cap/mongo-uri/src/index.cp
// Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser & formatter.
// MIT clean-room pure CP implementation.

/// A single MongoDB seedlist host and port (`port = 27017` by default for standard URIs, `0` for SRV).
pub struct MongoHost {
    pub host: string,
    pub port: u16,
}

/// Zero-copy parsed MongoDB connection string configuration (up to 8 replica-set hosts).
pub struct MongoUri {
    pub is_srv: bool,
    pub username: string,
    pub password: string,
    pub hosts: [8]MongoHost,
    pub host_count: usize,
    pub database: string,
    pub replica_set: string,
    pub auth_source: string,
    pub tls: bool,
    pub valid: bool,
}

fn muToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn muEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (muToLower(a[i]) != muToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn muStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return muEqIgnoreCase(s[0..prefix.len], prefix);
}

fn muContainsByte(s: []const u8, target: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn muParsePort(s: []const u8, out: *u16): bool {
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
    if (acc == (0 as u32)) {
        return false;
    }
    out.* = acc as u16;
    return true;
}

fn muWrite(out: []u8, pos: *usize, src: []const u8): bool {
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

fn muWriteU16(out: []u8, pos: *usize, val: u16): bool {
    if (val == (0 as u16)) {
        return muWrite(out, pos, "0");
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

fn muEmptyResult(): MongoUri {
    let h: [8]MongoHost = undefined;
    let idx: usize = 0;
    while (idx < 8) {
        h[idx] = MongoHost{ host: "", port: 0 as u16 };
        idx += 1;
    }
    return MongoUri{
        is_srv: false,
        username: "",
        password: "",
        hosts: h,
        host_count: 0,
        database: "",
        replica_set: "",
        auth_source: "",
        tls: false,
        valid: false,
    };
}

/// Parse a MongoDB connection string (`mongodb://` or `mongodb+srv://`).
/// - Supports replica-set seedlists (`host1:27017,host2:27018`), IPv6 `[::1]:27017`, and default port `27017`.
/// - Enforces MongoDB SRV spec: `mongodb+srv://` requires exactly one host without an explicit port and defaults `tls = true`.
/// - Extracts `database`, `replicaSet`, `authSource`, and `tls`/`ssl` query options.
pub fn parseMongoUri(uri: string): MongoUri {
    let res = muEmptyResult();

    let afterScheme: usize = 0;
    if (muStartsWithIgnoreCase(uri, "mongodb+srv://")) {
        res.is_srv = true;
        res.tls = true;
        afterScheme = 14;
    } else if (muStartsWithIgnoreCase(uri, "mongodb://")) {
        res.is_srv = false;
        afterScheme = 10;
    } else {
        return res;
    }

    const body = uri[afterScheme..uri.len];
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

    // Split authority and optional '/database'.
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
        res.database = mainPart[(slash + 1)..mainPart.len];
    }

    // Split optional userinfo'@'hostlist using the last '@'.
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

    let hostList = authority;
    if (hasAt) {
        const uinfo = authority[0..lastAt];
        hostList = authority[(lastAt + 1)..authority.len];
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
            res.username = uinfo;
        }
    }

    if (hostList.len == 0) {
        return res;
    }

    // Parse comma-separated hosts (up to 8).
    let hpos: usize = 0;
    while (hpos < hostList.len) {
        let comma = hpos;
        while (comma < hostList.len && hostList[comma] != (',' as u8)) {
            comma += 1;
        }
        const item = hostList[hpos..comma];
        if (item.len == 0 || res.host_count >= 8) {
            return res;
        }

        let hostName: []const u8 = "";
        let portVal: u16 = 27017 as u16;
        if (res.is_srv) {
            portVal = 0 as u16;
        }

        if (item[0] == ('[' as u8)) {
            if (res.is_srv) {
                return res;
            }
            let rbr: usize = 1;
            while (rbr < item.len && item[rbr] != (']' as u8)) {
                rbr += 1;
            }
            if (rbr >= item.len || rbr == 1) {
                return res;
            }
            hostName = item[1..rbr];
            if (rbr + 1 < item.len) {
                if (item[rbr + 1] != (':' as u8)) {
                    return res;
                }
                if (!muParsePort(item[(rbr + 2)..item.len], &portVal)) {
                    return res;
                }
            }
        } else {
            let colon = item.len;
            let ci: usize = 0;
            while (ci < item.len) {
                if (item[ci] == (':' as u8)) {
                    colon = ci;
                    break;
                }
                ci += 1;
            }
            if (colon < item.len) {
                // MongoDB SRV URIs forbid explicit port numbers.
                if (res.is_srv || colon == 0) {
                    return res;
                }
                hostName = item[0..colon];
                if (!muParsePort(item[(colon + 1)..item.len], &portVal)) {
                    return res;
                }
            } else {
                hostName = item;
            }
        }

        if (hostName.len == 0) {
            return res;
        }
        res.hosts[res.host_count] = MongoHost{
            host: hostName,
            port: portVal,
        };
        res.host_count += 1;

        if (comma < hostList.len) {
            if (comma + 1 == hostList.len) {
                return res;
            }
            hpos = comma + 1;
        } else {
            hpos = hostList.len;
        }
    }

    if (res.host_count == 0) {
        return res;
    }
    if (res.is_srv && res.host_count != 1) {
        return res;
    }

    // Parse core query options (`replicaSet`, `authSource`, `tls`, `ssl`).
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
                if (muEqIgnoreCase(k, "replicaset")) {
                    res.replica_set = v;
                } else if (muEqIgnoreCase(k, "authsource")) {
                    res.auth_source = v;
                } else if (muEqIgnoreCase(k, "tls") || muEqIgnoreCase(k, "ssl")) {
                    if (muEqIgnoreCase(v, "true") || muEqIgnoreCase(v, "1")) {
                        res.tls = true;
                    } else if (muEqIgnoreCase(v, "false") || muEqIgnoreCase(v, "0")) {
                        res.tls = false;
                    }
                }
            }
        }
    }

    res.valid = true;
    return res;
}

/// Zero-copy, case-insensitive lookup of any query option in a MongoDB URI
/// (e.g. `readPreference`, `maxPoolSize`, `retryWrites`). Returns `""` if absent.
pub fn getQueryOption(uri: string, key: string): string {
    if (key.len == 0) {
        return "";
    }
    let qmark: usize = 0;
    while (qmark < uri.len && uri[qmark] != ('?' as u8)) {
        qmark += 1;
    }
    if (qmark + 1 >= uri.len) {
        return "";
    }

    const q = uri[(qmark + 1)..uri.len];
    let pos: usize = 0;
    while (pos < q.len) {
        let amp = pos;
        while (amp < q.len && q[amp] != ('&' as u8)) {
            amp += 1;
        }
        const pair = q[pos..amp];
        if (amp < q.len) {
            pos = amp + 1;
        } else {
            pos = q.len;
        }

        let eq: usize = 0;
        while (eq < pair.len && pair[eq] != ('=' as u8)) {
            eq += 1;
        }
        if (eq < pair.len && muEqIgnoreCase(pair[0..eq], key)) {
            return pair[(eq + 1)..pair.len];
        }
    }
    return "";
}

/// Format a `MongoUri` back into a standard `mongodb://` or `mongodb+srv://` URI in `out`.
/// When `redact_password` is `true`, non-empty passwords are replaced with `***`.
/// Returns bytes written, or `0` on invalid config or insufficient `out` buffer.
pub fn formatMongoUri(cfg: MongoUri, redact_password: bool, out: []u8): usize {
    if (!cfg.valid || cfg.host_count == 0 || cfg.host_count > 8) {
        return 0;
    }

    let pos: usize = 0;
    if (cfg.is_srv) {
        if (!muWrite(out, &pos, "mongodb+srv://")) {
            return 0;
        }
    } else {
        if (!muWrite(out, &pos, "mongodb://")) {
            return 0;
        }
    }

    if (cfg.username.len > 0 || cfg.password.len > 0) {
        if (!muWrite(out, &pos, cfg.username)) {
            return 0;
        }
        if (cfg.password.len > 0) {
            if (!muWrite(out, &pos, ":")) {
                return 0;
            }
            if (redact_password) {
                if (!muWrite(out, &pos, "***")) {
                    return 0;
                }
            } else {
                if (!muWrite(out, &pos, cfg.password)) {
                    return 0;
                }
            }
        }
        if (!muWrite(out, &pos, "@")) {
            return 0;
        }
    }

    let i: usize = 0;
    while (i < cfg.host_count) {
        if (i > 0) {
            if (!muWrite(out, &pos, ",")) {
                return 0;
            }
        }
        const h = cfg.hosts[i];
        if (h.host.len == 0) {
            return 0;
        }
        if (muContainsByte(h.host, ':' as u8)) {
            if (!muWrite(out, &pos, "[") || !muWrite(out, &pos, h.host) || !muWrite(out, &pos, "]")) {
                return 0;
            }
        } else {
            if (!muWrite(out, &pos, h.host)) {
                return 0;
            }
        }
        if (!cfg.is_srv && h.port > (0 as u16)) {
            if (!muWrite(out, &pos, ":") || !muWriteU16(out, &pos, h.port)) {
                return 0;
            }
        }
        i += 1;
    }

    if (!muWrite(out, &pos, "/")) {
        return 0;
    }
    if (cfg.database.len > 0) {
        if (!muWrite(out, &pos, cfg.database)) {
            return 0;
        }
    }

    let hasParam = false;
    if (cfg.replica_set.len > 0) {
        if (!muWrite(out, &pos, "?replicaSet=") || !muWrite(out, &pos, cfg.replica_set)) {
            return 0;
        }
        hasParam = true;
    }
    if (cfg.auth_source.len > 0) {
        if (hasParam) {
            if (!muWrite(out, &pos, "&authSource=")) {
                return 0;
            }
        } else {
            if (!muWrite(out, &pos, "?authSource=")) {
                return 0;
            }
        }
        if (!muWrite(out, &pos, cfg.auth_source)) {
            return 0;
        }
        hasParam = true;
    }
    if (!cfg.is_srv && cfg.tls) {
        if (hasParam) {
            if (!muWrite(out, &pos, "&tls=true")) {
                return 0;
            }
        } else {
            if (!muWrite(out, &pos, "?tls=true")) {
                return 0;
            }
        }
    }

    return pos;
}

export {
    MongoHost,
    MongoUri,
    parseMongoUri,
    getQueryOption,
    formatMongoUri,
};
