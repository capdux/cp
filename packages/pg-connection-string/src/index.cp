// @cap/pg-dsn — Zero-heap PostgreSQL URI & libpq key=value DSN parser & builder
// MIT License (clean-room CP implementation)

pub const DEFAULT_PORT: u16 = 5432;

pub struct PgConfig {
    pub host: string,
    pub port: u16,
    pub user: string,
    pub password: string,
    pub database: string,
    pub sslmode: string,
    pub valid: bool,
}

fn makeInvalid() -> PgConfig {
    return PgConfig {
        host: "",
        port: DEFAULT_PORT,
        user: "",
        password: "",
        database: "",
        sslmode: "",
        valid: false,
    };
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == '\n' as u8 || c == '\r' as u8;
}

fn bytesEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: string, prefix: string) -> bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEq(s[0..prefix.len], prefix);
}

fn parsePortStrict(s: string) -> i32 {
    if (s.len == 0 || s.len > 5) {
        return -1;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return -1;
        }
        val = val * 10 + ((c - ('0' as u8)) as u32);
        if (val > 65535) {
            return -1;
        }
        i += 1;
    }
    if (val == 0) {
        return -1;
    }
    return val as i32;
}

pub fn isValidSslMode(mode: string) -> bool {
    return bytesEq(mode, "disable")
        || bytesEq(mode, "allow")
        || bytesEq(mode, "prefer")
        || bytesEq(mode, "require")
        || bytesEq(mode, "verify-ca")
        || bytesEq(mode, "verify-full");
}

pub fn parseUri(uri: string) -> PgConfig {
    let pos: usize = 0;
    if (startsWith(uri, "postgresql://")) {
        pos = 13;
    } else if (startsWith(uri, "postgres://")) {
        pos = 11;
    } else {
        return makeInvalid();
    }

    let cfg = PgConfig {
        host: "",
        port: DEFAULT_PORT,
        user: "",
        password: "",
        database: "",
        sslmode: "",
        valid: true,
    };

    // Split optional "?query"
    let q_pos: usize = pos;
    while (q_pos < uri.len && uri[q_pos] != '?' as u8) {
        q_pos += 1;
    }
    const before_q: string = uri[pos..q_pos];

    // Split authority and optional "/database"
    let slash: usize = 0;
    while (slash < before_q.len && before_q[slash] != '/' as u8) {
        slash += 1;
    }
    const authority: string = before_q[0..slash];
    if (slash + 1 < before_q.len) {
        cfg.database = before_q[(slash + 1)..before_q.len];
    }

    // Split optional "user[:pass]@" from host[:port] using LAST '@'
    let at_found: bool = false;
    let at_pos: usize = 0;
    let i: usize = authority.len;
    while (i > 0) {
        i -= 1;
        if (authority[i] == '@' as u8) {
            at_pos = i;
            at_found = true;
            break;
        }
    }

    let hostport: string = authority;
    if (at_found) {
        const userinfo: string = authority[0..at_pos];
        hostport = authority[(at_pos + 1)..authority.len];
        let colon: usize = 0;
        while (colon < userinfo.len && userinfo[colon] != ':' as u8) {
            colon += 1;
        }
        cfg.user = userinfo[0..colon];
        if (colon + 1 < userinfo.len) {
            cfg.password = userinfo[(colon + 1)..userinfo.len];
        }
    }

    // Parse host[:port] including IPv6 [::1]:5432
    if (hostport.len > 0) {
        if (hostport[0] == '[' as u8) {
            let close: usize = 1;
            while (close < hostport.len && hostport[close] != ']' as u8) {
                close += 1;
            }
            if (close >= hostport.len) {
                return makeInvalid();
            }
            cfg.host = hostport[1..close];
            if (close + 1 < hostport.len) {
                if (hostport[close + 1] != ':' as u8) {
                    return makeInvalid();
                }
                const port_i: i32 = parsePortStrict(hostport[(close + 2)..hostport.len]);
                if (port_i < 0) {
                    return makeInvalid();
                }
                cfg.port = port_i as u16;
            }
        } else {
            let colon: usize = 0;
            while (colon < hostport.len && hostport[colon] != ':' as u8) {
                colon += 1;
            }
            cfg.host = hostport[0..colon];
            if (colon < hostport.len) {
                const port_i: i32 = parsePortStrict(hostport[(colon + 1)..hostport.len]);
                if (port_i < 0) {
                    return makeInvalid();
                }
                cfg.port = port_i as u16;
            }
        }
    }

    // Parse optional query string (?k=v&...)
    if (q_pos + 1 < uri.len) {
        let qp: usize = q_pos + 1;
        while (qp < uri.len) {
            let amp: usize = qp;
            while (amp < uri.len && uri[amp] != '&' as u8) {
                amp += 1;
            }
            let eq: usize = qp;
            while (eq < amp && uri[eq] != '=' as u8) {
                eq += 1;
            }
            if (eq < amp) {
                const k: string = uri[qp..eq];
                const v: string = uri[(eq + 1)..amp];
                if (bytesEq(k, "sslmode")) {
                    if (!isValidSslMode(v)) {
                        return makeInvalid();
                    }
                    cfg.sslmode = v;
                } else if (bytesEq(k, "port")) {
                    const port_i: i32 = parsePortStrict(v);
                    if (port_i < 0) {
                        return makeInvalid();
                    }
                    cfg.port = port_i as u16;
                } else if (bytesEq(k, "user")) {
                    cfg.user = v;
                } else if (bytesEq(k, "password")) {
                    cfg.password = v;
                } else if (bytesEq(k, "dbname")) {
                    cfg.database = v;
                } else if (bytesEq(k, "host")) {
                    cfg.host = v;
                }
            }
            qp = amp;
            if (qp < uri.len && uri[qp] == '&' as u8) {
                qp += 1;
            }
        }
    }

    return cfg;
}

pub fn parseKeyValue(dsn: string) -> PgConfig {
    let cfg = PgConfig {
        host: "",
        port: DEFAULT_PORT,
        user: "",
        password: "",
        database: "",
        sslmode: "",
        valid: true,
    };

    let pos: usize = 0;
    let pairs: usize = 0;

    while (pos < dsn.len) {
        while (pos < dsn.len && isWs(dsn[pos])) {
            pos += 1;
        }
        if (pos >= dsn.len) {
            break;
        }

        const k_start: usize = pos;
        while (pos < dsn.len && dsn[pos] != '=' as u8 && !isWs(dsn[pos])) {
            pos += 1;
        }
        const k_end: usize = pos;
        while (pos < dsn.len && isWs(dsn[pos])) {
            pos += 1;
        }
        if (k_end <= k_start || pos >= dsn.len || dsn[pos] != '=' as u8) {
            return makeInvalid();
        }
        pos += 1; // skip '='
        while (pos < dsn.len && isWs(dsn[pos])) {
            pos += 1;
        }

        let v_start: usize = pos;
        let v_end: usize = pos;
        if (pos < dsn.len && dsn[pos] == '\'' as u8) {
            pos += 1;
            v_start = pos;
            let closed: bool = false;
            while (pos < dsn.len) {
                if (dsn[pos] == '\\' as u8 && pos + 1 < dsn.len) {
                    pos += 2;
                    continue;
                }
                if (dsn[pos] == '\'' as u8) {
                    v_end = pos;
                    pos += 1;
                    closed = true;
                    break;
                }
                pos += 1;
            }
            if (!closed) {
                return makeInvalid();
            }
        } else {
            while (pos < dsn.len && !isWs(dsn[pos])) {
                pos += 1;
            }
            v_end = pos;
        }

        const k: string = dsn[k_start..k_end];
        const v: string = dsn[v_start..v_end];
        if (bytesEq(k, "host")) {
            cfg.host = v;
        } else if (bytesEq(k, "port")) {
            const port_i: i32 = parsePortStrict(v);
            if (port_i < 0) {
                return makeInvalid();
            }
            cfg.port = port_i as u16;
        } else if (bytesEq(k, "user")) {
            cfg.user = v;
        } else if (bytesEq(k, "password")) {
            cfg.password = v;
        } else if (bytesEq(k, "dbname")) {
            cfg.database = v;
        } else if (bytesEq(k, "sslmode")) {
            if (!isValidSslMode(v)) {
                return makeInvalid();
            }
            cfg.sslmode = v;
        }
        pairs += 1;
    }

    if (pairs == 0) {
        return makeInvalid();
    }
    return cfg;
}

pub fn parse(input: string) -> PgConfig {
    let s: usize = 0;
    let e: usize = input.len;
    while (s < e && isWs(input[s])) {
        s += 1;
    }
    while (e > s && isWs(input[e - 1])) {
        e -= 1;
    }
    if (s >= e) {
        return makeInvalid();
    }
    const trimmed: string = input[s..e];
    if (startsWith(trimmed, "postgresql://") || startsWith(trimmed, "postgres://")) {
        return parseUri(trimmed);
    }
    return parseKeyValue(trimmed);
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

fn appendU16(out: []u8, pos: usize, v: u16) -> i64 {
    let digits: [6]u8 = undefined;
    let count: usize = 0;
    if (v == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u16 = v;
        while (cur > 0) {
            digits[count] = ('0' as u8) + ((cur % 10) as u8);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > out.len - pos) {
        return -1;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p as i64;
}

fn containsColon(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ':' as u8) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn formatUri(cfg: PgConfig, include_password: bool, out: []u8) -> usize {
    let p: i64 = appendSlice(out, 0, "postgresql://");
    if (p < 0) {
        return 0;
    }
    if (cfg.user.len > 0) {
        p = appendSlice(out, p as usize, cfg.user);
        if (p < 0) {
            return 0;
        }
        if (include_password && cfg.password.len > 0) {
            p = appendSlice(out, p as usize, ":");
            if (p < 0) {
                return 0;
            }
            p = appendSlice(out, p as usize, cfg.password);
            if (p < 0) {
                return 0;
            }
        }
        p = appendSlice(out, p as usize, "@");
        if (p < 0) {
            return 0;
        }
    }

    if (cfg.host.len > 0) {
        const is_v6: bool = containsColon(cfg.host);
        if (is_v6) {
            p = appendSlice(out, p as usize, "[");
            if (p < 0) {
                return 0;
            }
        }
        p = appendSlice(out, p as usize, cfg.host);
        if (p < 0) {
            return 0;
        }
        if (is_v6) {
            p = appendSlice(out, p as usize, "]");
            if (p < 0) {
                return 0;
            }
        }
    }

    if (cfg.port != 0) {
        p = appendSlice(out, p as usize, ":");
        if (p < 0) {
            return 0;
        }
        p = appendU16(out, p as usize, cfg.port);
        if (p < 0) {
            return 0;
        }
    }

    if (cfg.database.len > 0) {
        p = appendSlice(out, p as usize, "/");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, cfg.database);
        if (p < 0) {
            return 0;
        }
    }

    if (cfg.sslmode.len > 0) {
        p = appendSlice(out, p as usize, "?sslmode=");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, cfg.sslmode);
        if (p < 0) {
            return 0;
        }
    }

    return p as usize;
}
