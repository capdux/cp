// @cap/mysql-dsn — Zero-heap MySQL / MariaDB / TiDB DSN parser & redacting formatter
// Clean-room MIT implementation for CP (compatible with npm mysql2 & Rust mysql).

pub struct MysqlConfig {
    pub tls: bool,
    pub host: string,
    pub port: u16,
    pub user: string,
    pub password: string,
    pub database: string,
    pub charset: string,
    pub valid: bool,
}

fn eqBytes(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) { return false; }
    return eqBytes(s[0..prefix.len], prefix);
}

fn parsePort(s: string): i32 {
    if (s.len == 0 || s.len > 5) { return -1; }
    let v: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = s[i];
        if (c < '0' || c > '9') { return -1; }
        v = v * 10 + ((c - '0') as i32);
        i += 1;
    }
    if (v <= 0 || v > 65535) { return -1; }
    return v;
}

fn indexOf(s: string, ch: u8): i64 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) { return i as i64; }
        i += 1;
    }
    return -1;
}

fn lastIndexOf(s: string, ch: u8): i64 {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) { return i as i64; }
    }
    return -1;
}

fn applyQueryParams(query: string, cfg: *MysqlConfig) {
    let i: usize = 0;
    while (i < query.len) {
        let start: usize = i;
        while (i < query.len && query[i] != '&') { i += 1; }
        let seg: string = query[start..i];
        let eq: i64 = indexOf(seg, '=');
        if (eq > 0) {
            let k: string = seg[0..(eq as usize)];
            let v: string = seg[((eq as usize) + 1)..seg.len];
            if (eqBytes(k, "charset")) {
                cfg.charset = v;
            } else if (eqBytes(k, "tls") || eqBytes(k, "ssl") || eqBytes(k, "ssl-mode")) {
                if (eqBytes(v, "true") || eqBytes(v, "1") || eqBytes(v, "REQUIRED") || eqBytes(v, "VERIFY_IDENTITY")) {
                    cfg.tls = true;
                }
            }
        }
        if (i < query.len) { i += 1; }
    }
}

fn splitHostPort(hp: string, out_host: *string, out_port: *u16): bool {
    if (hp.len == 0) { return false; }
    if (hp[0] == '[') {
        let rb: i64 = indexOf(hp, ']');
        if (rb <= 1) { return false; }
        let rbu: usize = rb as usize;
        out_host.* = hp[1..rbu];
        if (rbu + 1 == hp.len) {
            out_port.* = 3306;
            return true;
        }
        if (hp[rbu + 1] != ':') { return false; }
        let p: i32 = parsePort(hp[(rbu + 2)..hp.len]);
        if (p < 0) { return false; }
        out_port.* = p as u16;
        return true;
    }
    let colon: i64 = lastIndexOf(hp, ':');
    if (colon < 0) {
        out_host.* = hp;
        out_port.* = 3306;
        return true;
    }
    let cu: usize = colon as usize;
    if (cu == 0) { return false; }
    let p: i32 = parsePort(hp[(cu + 1)..hp.len]);
    if (p < 0) { return false; }
    out_host.* = hp[0..cu];
    out_port.* = p as u16;
    return true;
}

/// Parse `mysql://[user[:pass]@]host[:port][/db][?params]` or `mysqls://...` URI.
pub fn parseUri(uri: string): MysqlConfig {
    let invalid: MysqlConfig = MysqlConfig {
        tls: false,
        host: "",
        port: 3306,
        user: "",
        password: "",
        database: "",
        charset: "utf8mb4",
        valid: false,
    };

    let tls: bool = false;
    let rest: string = uri;
    if (startsWith(uri, "mysql://")) {
        rest = uri[8..uri.len];
    } else if (startsWith(uri, "mysqls://")) {
        tls = true;
        rest = uri[9..uri.len];
    } else if (startsWith(uri, "mariadb://")) {
        rest = uri[10..uri.len];
    } else {
        return invalid;
    }

    let query: string = "";
    let qm: i64 = indexOf(rest, '?');
    if (qm >= 0) {
        query = rest[((qm as usize) + 1)..rest.len];
        rest = rest[0..(qm as usize)];
    }

    let db: string = "";
    let slash: i64 = indexOf(rest, '/');
    let authority: string = rest;
    if (slash >= 0) {
        authority = rest[0..(slash as usize)];
        db = rest[((slash as usize) + 1)..rest.len];
    }

    let user: string = "";
    let pass: string = "";
    let hostport: string = authority;
    let at: i64 = lastIndexOf(authority, '@');
    if (at >= 0) {
        let userinfo: string = authority[0..(at as usize)];
        hostport = authority[((at as usize) + 1)..authority.len];
        let colon: i64 = indexOf(userinfo, ':');
        if (colon >= 0) {
            user = userinfo[0..(colon as usize)];
            pass = userinfo[((colon as usize) + 1)..userinfo.len];
        } else {
            user = userinfo;
        }
    }

    let host: string = "";
    let port: u16 = 3306;
    // Inline host/port split to avoid *string out-param
    if (hostport.len == 0) { return invalid; }
    if (hostport[0] == '[') {
        let rb: i64 = indexOf(hostport, ']');
        if (rb <= 1) { return invalid; }
        let rbu: usize = rb as usize;
        host = hostport[1..rbu];
        if (rbu + 1 < hostport.len) {
            if (hostport[rbu + 1] != ':') { return invalid; }
            let p: i32 = parsePort(hostport[(rbu + 2)..hostport.len]);
            if (p < 0) { return invalid; }
            port = p as u16;
        }
    } else {
        let colon: i64 = lastIndexOf(hostport, ':');
        if (colon < 0) {
            host = hostport;
        } else {
            let cu: usize = colon as usize;
            if (cu == 0) { return invalid; }
            let p: i32 = parsePort(hostport[(cu + 1)..hostport.len]);
            if (p < 0) { return invalid; }
            host = hostport[0..cu];
            port = p as u16;
        }
    }

    let cfg: MysqlConfig = MysqlConfig {
        tls: tls,
        host: host,
        port: port,
        user: user,
        password: pass,
        database: db,
        charset: "utf8mb4",
        valid: true,
    };
    if (query.len > 0) {
        applyQueryParams(query, &cfg);
    }
    return cfg;
}

/// Parse Go-MySQL style DSN: `user:pass@tcp(host:3306)/dbname?charset=utf8mb4`.
pub fn parseGoDsn(dsn: string): MysqlConfig {
    let invalid: MysqlConfig = MysqlConfig {
        tls: false,
        host: "",
        port: 3306,
        user: "",
        password: "",
        database: "",
        charset: "utf8mb4",
        valid: false,
    };

    let rest: string = dsn;
    let query: string = "";
    let qm: i64 = indexOf(rest, '?');
    if (qm >= 0) {
        query = rest[((qm as usize) + 1)..rest.len];
        rest = rest[0..(qm as usize)];
    }

    let slash: i64 = indexOf(rest, '/');
    if (slash < 0) { return invalid; }
    let head: string = rest[0..(slash as usize)];
    let db: string = rest[((slash as usize) + 1)..rest.len];

    let user: string = "";
    let pass: string = "";
    let addr_part: string = head;
    let at: i64 = lastIndexOf(head, '@');
    if (at >= 0) {
        let ui: string = head[0..(at as usize)];
        addr_part = head[((at as usize) + 1)..head.len];
        let colon: i64 = indexOf(ui, ':');
        if (colon >= 0) {
            user = ui[0..(colon as usize)];
            pass = ui[((colon as usize) + 1)..ui.len];
        } else {
            user = ui;
        }
    }

    if (!startsWith(addr_part, "tcp(") || addr_part.len < 6 || addr_part[addr_part.len - 1] != ')') {
        return invalid;
    }
    let hp: string = addr_part[4..(addr_part.len - 1)];
    if (hp.len == 0) { return invalid; }

    let host: string = hp;
    let port: u16 = 3306;
    let colon: i64 = lastIndexOf(hp, ':');
    if (colon >= 0) {
        let cu: usize = colon as usize;
        if (cu == 0) { return invalid; }
        let p: i32 = parsePort(hp[(cu + 1)..hp.len]);
        if (p < 0) { return invalid; }
        host = hp[0..cu];
        port = p as u16;
    }

    let cfg: MysqlConfig = MysqlConfig {
        tls: false,
        host: host,
        port: port,
        user: user,
        password: pass,
        database: db,
        charset: "utf8mb4",
        valid: true,
    };
    if (query.len > 0) {
        applyQueryParams(query, &cfg);
    }
    return cfg;
}

/// Auto-detect and parse either `mysql://` URI or Go-MySQL `user:pass@tcp(host:port)/db` DSN.
pub fn parse(input: string): MysqlConfig {
    if (startsWith(input, "mysql://") || startsWith(input, "mysqls://") || startsWith(input, "mariadb://")) {
        return parseUri(input);
    }
    return parseGoDsn(input);
}

fn appendStr(dst: []u8, pos: usize, src: string): usize {
    if (pos + src.len > dst.len) { return 0; }
    let i: usize = 0;
    while (i < src.len) { dst[pos + i] = src[i]; i += 1; }
    return pos + src.len;
}

fn appendU16(dst: []u8, pos: usize, val: u16): usize {
    let tmp: [8]u8 = undefined;
    let n: usize = 0;
    let v: u16 = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
            n += 1;
        }
    }
    if (pos + n > dst.len) { return 0; }
    let i: usize = 0;
    while (i < n) {
        dst[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// Format `MysqlConfig` as `mysql://...` URI (`redact_password = true` replaces password with `***`).
pub fn formatUri(cfg: MysqlConfig, redact_password: bool, out: []u8): usize {
    if (!cfg.valid || cfg.host.len == 0) { return 0; }
    let scheme: string = "mysql://";
    if (cfg.tls) { scheme = "mysqls://"; }
    let pos: usize = appendStr(out, 0, scheme);
    if (pos == 0) { return 0; }

    if (cfg.user.len > 0) {
        pos = appendStr(out, pos, cfg.user);
        if (pos == 0) { return 0; }
        if (cfg.password.len > 0) {
            pos = appendStr(out, pos, ":");
            if (pos == 0) { return 0; }
            if (redact_password) {
                pos = appendStr(out, pos, "***");
            } else {
                pos = appendStr(out, pos, cfg.password);
            }
            if (pos == 0) { return 0; }
        }
        pos = appendStr(out, pos, "@");
        if (pos == 0) { return 0; }
    }

    let is_v6: bool = indexOf(cfg.host, ':') >= 0;
    if (is_v6) {
        pos = appendStr(out, pos, "[");
        if (pos == 0) { return 0; }
    }
    pos = appendStr(out, pos, cfg.host);
    if (pos == 0) { return 0; }
    if (is_v6) {
        pos = appendStr(out, pos, "]");
        if (pos == 0) { return 0; }
    }

    pos = appendStr(out, pos, ":");
    if (pos == 0) { return 0; }
    pos = appendU16(out, pos, cfg.port);
    if (pos == 0) { return 0; }

    if (cfg.database.len > 0) {
        pos = appendStr(out, pos, "/");
        if (pos == 0) { return 0; }
        pos = appendStr(out, pos, cfg.database);
        if (pos == 0) { return 0; }
    }
    return pos;
}
