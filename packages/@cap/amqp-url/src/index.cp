// @cap/amqp-url — Zero-allocation AMQP 0-9-1 / RabbitMQ URI Parser & Redacting Formatter
// Clean-room MIT implementation in pure CP.

pub struct AmqpConfig {
    pub tls: bool,
    pub host: string,
    pub port: u16,
    pub username: string,
    pub password: string,
    pub vhost: string,
    pub heartbeat: u16,
    pub valid: bool,
}

fn invalidAmqp() -> AmqpConfig {
    return AmqpConfig {
        tls: false,
        host: "",
        port: 0,
        username: "",
        password: "",
        vhost: "",
        heartbeat: 0,
        valid: false,
    };
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn eqIgnoreCase(a: string, lower_b: string) -> bool {
    if (a.len != lower_b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != lower_b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseU16Strict(s: string) -> i32 {
    if (s.len == 0 || s.len > 5) {
        return -1;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        acc = acc * 10 + ((ch - 48) as u32);
        if (acc > 65535) {
            return -1;
        }
        i += 1;
    }
    return acc as i32;
}

/// Parses an AMQP 0-9-1 / RabbitMQ connection URI (`amqp://` or `amqps://`).
/// Defaults: port `5672` (`5671` for `amqps`), `username = "guest"`, `password = "guest"`, `vhost = "/"`.
/// Handles `%2F` root vhost, IPv6 `[::1]:5672`, and `?heartbeat=<secs>`.
pub fn parseAmqpUrl(url: string) -> AmqpConfig {
    let a: usize = 0;
    let b: usize = url.len;
    while (a < b && isSpace(url[a])) {
        a += 1;
    }
    while (b > a && isSpace(url[b - 1])) {
        b -= 1;
    }
    const s = url[a..b];

    let tls = false;
    let rest_start: usize = 0;
    if (s.len >= 8 && eqIgnoreCase(s[0..8], "amqps://")) {
        tls = true;
        rest_start = 8;
    } else if (s.len >= 7 && eqIgnoreCase(s[0..7], "amqp://")) {
        tls = false;
        rest_start = 7;
    } else {
        return invalidAmqp();
    }

    let port: u16 = 5672;
    if (tls) {
        port = 5671;
    }

    let rest_end = s.len;
    let heartbeat: u16 = 0;

    // 1. Strip optional ?query
    let q_idx = rest_start;
    while (q_idx < rest_end && s[q_idx] != 63) {
        q_idx += 1;
    }
    if (q_idx < rest_end) {
        const query = s[q_idx + 1..rest_end];
        rest_end = q_idx;

        let qp: usize = 0;
        while (qp < query.len) {
            const pstart = qp;
            while (qp < query.len && query[qp] != 38) {
                qp += 1;
            }
            const pair = query[pstart..qp];
            if (qp < query.len && query[qp] == 38) {
                qp += 1;
            }
            let eq: usize = 0;
            while (eq < pair.len && pair[eq] != 61) {
                eq += 1;
            }
            if (eq < pair.len && eqIgnoreCase(pair[0..eq], "heartbeat")) {
                const hb = parseU16Strict(pair[eq + 1..pair.len]);
                if (hb < 0) {
                    return invalidAmqp();
                }
                heartbeat = hb as u16;
            }
        }
    }

    // 2. Split authority and optional /vhost
    let slash_idx = rest_start;
    while (slash_idx < rest_end && s[slash_idx] != 47) {
        slash_idx += 1;
    }

    let vhost: string = "/";
    if (slash_idx < rest_end) {
        const raw_vhost = s[slash_idx + 1..rest_end];
        if (raw_vhost.len == 0 || eqIgnoreCase(raw_vhost, "%2f")) {
            vhost = "/";
        } else {
            vhost = raw_vhost;
        }
    }

    const authority = s[rest_start..slash_idx];
    if (authority.len == 0) {
        return invalidAmqp();
    }

    // 3. Split optional user:pass@ from host[:port]
    let username: string = "guest";
    let password: string = "guest";
    let hostport = authority;

    let at_idx = authority.len;
    let scan = authority.len;
    while (scan > 0) {
        scan -= 1;
        if (authority[scan] == 64) {
            at_idx = scan;
            break;
        }
    }
    if (at_idx < authority.len) {
        const userinfo = authority[0..at_idx];
        hostport = authority[at_idx + 1..authority.len];
        let colon: usize = 0;
        while (colon < userinfo.len && userinfo[colon] != 58) {
            colon += 1;
        }
        if (colon < userinfo.len) {
            username = userinfo[0..colon];
            password = userinfo[colon + 1..userinfo.len];
        } else {
            username = userinfo;
            password = "";
        }
    }

    if (hostport.len == 0) {
        return invalidAmqp();
    }

    // 4. Parse host (IPv6 bracketed or regular) & optional :port
    let host: string = "";
    if (hostport[0] == 91) {
        // '[' IPv6 literal
        let close: usize = 1;
        while (close < hostport.len && hostport[close] != 93) {
            close += 1;
        }
        if (close >= hostport.len || close == 1) {
            return invalidAmqp();
        }
        host = hostport[1..close];
        if (close + 1 < hostport.len) {
            if (hostport[close + 1] != 58) {
                return invalidAmqp();
            }
            const parsed_port = parseU16Strict(hostport[close + 2..hostport.len]);
            if (parsed_port <= 0) {
                return invalidAmqp();
            }
            port = parsed_port as u16;
        }
    } else {
        let colon: usize = 0;
        while (colon < hostport.len && hostport[colon] != 58) {
            colon += 1;
        }
        if (colon < hostport.len) {
            host = hostport[0..colon];
            const parsed_port = parseU16Strict(hostport[colon + 1..hostport.len]);
            if (parsed_port <= 0) {
                return invalidAmqp();
            }
            port = parsed_port as u16;
        } else {
            host = hostport;
        }
    }

    if (host.len == 0) {
        return invalidAmqp();
    }

    return AmqpConfig {
        tls: tls,
        host: host,
        port: port,
        username: username,
        password: password,
        vhost: vhost,
        heartbeat: heartbeat,
        valid: true,
    };
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn pushU16(dst: []u8, pos: usize, val: u16) -> i64 {
    let rev: [5]u8 = undefined;
    let n: usize = 0;
    let v = val as u32;
    if (v == 0) {
        rev[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            rev[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    if (pos + n > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < n) {
        dst[pos + i] = rev[n - 1 - i];
        i += 1;
    }
    return (pos + n) as i64;
}

fn containsColon(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 58) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Formats an `AmqpConfig` as a standard AMQP URI into `out`.
/// When `redact_password` is true, non-empty passwords are replaced with `***`.
/// Returns bytes written, or `0` if invalid or `out` is too small.
pub fn formatAmqpUrl(cfg: AmqpConfig, redact_password: bool, out: []u8) -> usize {
    if (cfg.host.len == 0 || cfg.port == 0) {
        return 0;
    }

    let p: i64 = 0;
    if (cfg.tls) {
        p = pushSlice(out, 0, "amqps://");
    } else {
        p = pushSlice(out, 0, "amqp://");
    }
    if (p < 0) { return 0; }

    if (cfg.username.len > 0) {
        p = pushSlice(out, p as usize, cfg.username);
        if (p < 0) { return 0; }
        if (cfg.password.len > 0) {
            p = pushSlice(out, p as usize, ":");
            if (p < 0) { return 0; }
            if (redact_password) {
                p = pushSlice(out, p as usize, "***");
            } else {
                p = pushSlice(out, p as usize, cfg.password);
            }
            if (p < 0) { return 0; }
        }
        p = pushSlice(out, p as usize, "@");
        if (p < 0) { return 0; }
    }

    if (containsColon(cfg.host)) {
        p = pushSlice(out, p as usize, "[");
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, cfg.host);
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, "]");
        if (p < 0) { return 0; }
    } else {
        p = pushSlice(out, p as usize, cfg.host);
        if (p < 0) { return 0; }
    }

    p = pushSlice(out, p as usize, ":");
    if (p < 0) { return 0; }
    p = pushU16(out, p as usize, cfg.port);
    if (p < 0) { return 0; }

    p = pushSlice(out, p as usize, "/");
    if (p < 0) { return 0; }
    if (cfg.vhost.len == 1 && cfg.vhost[0] == 47) {
        p = pushSlice(out, p as usize, "%2F");
        if (p < 0) { return 0; }
    } else if (cfg.vhost.len > 0) {
        p = pushSlice(out, p as usize, cfg.vhost);
        if (p < 0) { return 0; }
    }

    if (cfg.heartbeat > 0) {
        p = pushSlice(out, p as usize, "?heartbeat=");
        if (p < 0) { return 0; }
        p = pushU16(out, p as usize, cfg.heartbeat);
        if (p < 0) { return 0; }
    }

    return p as usize;
}
