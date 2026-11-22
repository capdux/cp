// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T11:12:13Z (RFC 9110 Host Header Validator & Parser)

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isValidRegNameChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~';
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn parseStrictPort(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) { return null; }
    if (s[0] == '0') { return null; } // 禁止端口 0 与前导零（如 080）
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) { return null; }
        acc = acc * 10 + @as(u32, s[i] - '0');
        i += 1;
    }
    if (acc == 0 || acc > 65535) { return null; }
    return @as(u16, @intCast(acc));
}

fn isValidIpv6Inner(inner: []const u8): bool {
    if (inner.len < 2) { return false; }
    let has_colon = false;
    let i: usize = 0;
    while (i < inner.len) {
        const c = inner[i];
        if (c == ':') {
            has_colon = true;
        } else if (!isHexDigit(c) && c != '.') {
            return false;
        }
        i += 1;
    }
    return has_colon;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeLowerAscii(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            let c = s[i];
            if (c >= 'A' && c <= 'Z') { c += 32; }
            _.writeByte(c);
            i += 1;
        }
    }

    fn writeU16(*_, val: u16) {
        let digits: [8]u8 = [0; 8];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 8) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// RFC 9110 §7.2 HTTP `Host` / `:authority` 解析结果（零堆分配）
/// IPv6 时 `host` 保留方括号（如 `"[::1]"`），可通过 `unbracketedHost()` 获取裸地址
pub struct HostHeader {
    host: []const u8 = "",
    port: u16? = null,
    is_ipv6: bool = false,

    /// 返回去除 IPv6 方括号后的裸主机名（如 `"[::1]"` -> `"::1"`）
    fn unbracketedHost(): []const u8 {
        if (_.is_ipv6 && _.host.len >= 2 && _.host[0] == '[' && _.host[_.host.len - 1] == ']') {
            return _.host[1..(_.host.len - 1)];
        }
        return _.host;
    }

    /// 严格按 RFC 9110 §7.2 解析并校验 `Host` / `:authority` 头部值
    /// 拒绝空串、空白、userinfo `@`、路径 `/`、`?`、`#`、多余冒号、前导零/越界端口
    static fn parse(raw: []const u8): HostHeader? {
        if (raw.len == 0) { return null; }

        // 快速拦截空白、控制字符及非 authority 分隔符
        let i: usize = 0;
        while (i < raw.len) {
            const c = raw[i];
            if (c <= 0x20 || c == 0x7F || c == '@' || c == '/' || c == '?' || c == '#' || c == '\\') {
                return null;
            }
            i += 1;
        }

        // IPv6 字面量: [addr] 或 [addr]:port
        if (raw[0] == '[') {
            let close_idx: usize? = null;
            let k: usize = 1;
            while (k < raw.len) {
                if (raw[k] == ']') {
                    close_idx = k;
                    break;
                }
                if (raw[k] == '[') {
                    return null;
                }
                k += 1;
            }
            if (rb := close_idx) {
                const inner = raw[1..rb];
                if (!isValidIpv6Inner(inner)) {
                    return null;
                }
                const host_slice = raw[0..(rb + 1)];
                if (rb + 1 == raw.len) {
                    return HostHeader{
                        host: host_slice,
                        port: null,
                        is_ipv6: true,
                    };
                }
                if (raw[rb + 1] != ':') {
                    return null;
                }
                const p = parseStrictPort(raw[(rb + 2)..raw.len]) ?? return null;
                return HostHeader{
                    host: host_slice,
                    port: p,
                    is_ipv6: true,
                };
            }
            return null;
        }

        // reg-name / IPv4: host 或 host:port
        let colon_idx: usize? = null;
        let m: usize = 0;
        while (m < raw.len) {
            const c = raw[m];
            if (c == '[' || c == ']') { return null; }
            if (c == ':') {
                if (colon_idx != null) {
                    return null; // 多余冒号
                }
                colon_idx = m;
            }
            m += 1;
        }

        let host_part = raw;
        let parsed_port: u16? = null;
        if (ci := colon_idx) {
            host_part = raw[0..ci];
            parsed_port = parseStrictPort(raw[(ci + 1)..raw.len]) ?? return null;
        }

        if (host_part.len == 0) { return null; }
        let h: usize = 0;
        while (h < host_part.len) {
            if (!isValidRegNameChar(host_part[h])) {
                return null;
            }
            h += 1;
        }

        return HostHeader{
            host: host_part,
            port: parsed_port,
            is_ipv6: false,
        };
    }

    /// 返回生效端口（无显式端口时 TLS -> 443，明文 HTTP -> 80）
    fn effectivePort(is_tls: bool): u16 {
        if (p := _.port) {
            return p;
        }
        return if (is_tls) 443 else 80;
    }

    /// 判断是否为本地回环主机（`localhost` / `127.0.0.1` / `[::1]`，大小写无关）
    fn isLoopback(): bool {
        const h = _.unbracketedHost();
        return eqlIgnoreCaseAscii(h, "localhost") ||
            eqlIgnoreCaseAscii(h, "127.0.0.1") ||
            eqlIgnoreCaseAscii(h, "::1") ||
            eqlIgnoreCaseAscii(h, "0:0:0:0:0:0:0:1");
    }

    /// 规范化输出：主机名小写化并在端口等于协议默认端口（HTTP=80 / HTTPS=443）时自动省略
    fn formatCanonical(is_tls: bool, out: []u8): ([]const u8)? {
        let w = OutWriter{ buf: out };
        w.writeLowerAscii(_.host);
        if (p := _.port) {
            const def_port: u16 = if (is_tls) 443 else 80;
            if (p != def_port) {
                w.writeByte(':');
                w.writeU16(p);
            }
        }
        return w.finish();
    }
}
