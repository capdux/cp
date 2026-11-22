// Source: Absorbed from /data/dev/bun/src/http/ProxyTunnel.cp | Synced: 2026-11-22T11:12:13Z

/// IPv4 地址（4 字节网络序，纯 CP 零堆分配）
export struct Ipv4Addr {
    octets: [4]u8;

    /// 严格解析点分十进制 IPv4 地址（拒绝前导零、空段、超 255 或非法字符）
    fn parse(s: []const u8): Ipv4Addr? {
        if (s.len < 7 || s.len > 15) {
            return null;
        }
        let octets: [4]u8 = [0, 0, 0, 0];
        let part: usize = 0;
        let i: usize = 0;

        while (part < 4) {
            if (i >= s.len) {
                return null;
            }
            const seg_start = i;
            let val: u32 = 0;
            while (i < s.len && s[i] != '.') {
                const c = s[i];
                if (c < '0' || c > '9') {
                    return null;
                }
                val = val * 10 + u32(c - '0');
                if (val > 255) {
                    return null;
                }
                i += 1;
            }
            const seg_len = i - seg_start;
            if (seg_len == 0 || (seg_len > 1 && s[seg_start] == '0')) {
                return null;
            }
            octets[part] = u8(val);
            part += 1;

            if (part < 4) {
                if (i >= s.len || s[i] != '.') {
                    return null;
                }
                i += 1;
            }
        }

        if (i != s.len) {
            return null;
        }
        return Ipv4Addr{ octets: octets };
    }

    /// 转换为大端无符号 32 位整数
    fn toU32(): u32 {
        return (u32(_.octets[0]) << 24)
            | (u32(_.octets[1]) << 16)
            | (u32(_.octets[2]) << 8)
            | u32(_.octets[3]);
    }

    /// 是否为回环地址（127.0.0.0/8）
    fn isLoopback(): bool {
        return _.octets[0] == 127;
    }

    /// 是否为 RFC 1918 私网地址（10/8、172.16/12、192.168/16）
    fn isPrivate(): bool {
        if (_.octets[0] == 10) {
            return true;
        }
        if (_.octets[0] == 172 && _.octets[1] >= 16 && _.octets[1] <= 31) {
            return true;
        }
        if (_.octets[0] == 192 && _.octets[1] == 168) {
            return true;
        }
        return false;
    }

    /// 比较两个 IPv4 地址是否相等
    fn eql(other: Ipv4Addr): bool {
        return _.toU32() == other.toU32();
    }
}

/// IPv4 CIDR 网段（如 10.0.0.0/8、192.168.1.0/24）
export struct Ipv4Cidr {
    network: u32;
    prefix_len: u8;

    /// 解析形如 `a.b.c.d/prefix` 的 IPv4 CIDR 表达式（prefix 范围 0..32）
    fn parse(s: []const u8): Ipv4Cidr? {
        const slash_idx = findByte(s, '/') ?? return null;
        const ip_part = s[0..slash_idx];
        const prefix_part = s[slash_idx + 1..];
        if (prefix_part.len == 0 || prefix_part.len > 2) {
            return null;
        }
        if (prefix_part.len > 1 && prefix_part[0] == '0') {
            return null;
        }

        let prefix: u32 = 0;
        for (c of prefix_part) {
            if (c < '0' || c > '9') {
                return null;
            }
            prefix = prefix * 10 + u32(c - '0');
        }
        if (prefix > 32) {
            return null;
        }

        const ip = Ipv4Addr.parse(ip_part) ?? return null;
        const p: u8 = u8(prefix);
        const mask = cidrMask(p);
        return Ipv4Cidr{
            network: ip.toU32() & mask,
            prefix_len: p,
        };
    }

    /// 判断给定 IPv4 地址是否落在该 CIDR 网段内
    fn contains(ip: Ipv4Addr): bool {
        if (_.prefix_len > 32) {
            return false;
        }
        if (_.prefix_len == 0) {
            return true;
        }
        const mask = cidrMask(_.prefix_len);
        return (ip.toU32() & mask) == _.network;
    }
}

fn cidrMask(prefix_len: u8): u32 {
    if (prefix_len == 0) {
        return 0;
    }
    if (prefix_len >= 32) {
        return 0xFFFF_FFFF;
    }
    const shift: u5 = @intCast(usize(32) - usize(prefix_len));
    return (~u32(0)) << shift;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn countByte(s: []const u8, needle: u8): usize {
    let n: usize = 0;
    for (c of s) {
        if (c == needle) {
            n += 1;
        }
    }
    return n;
}

fn parsePort(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    let val: u32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + u32(c - '0');
        if (val > 65535) {
            return null;
        }
    }
    return u16(val);
}

fn stripTrailingDot(s: []const u8): []const u8 {
    if (s.len > 0 && s[s.len - 1] == '.') {
        return s[0..s.len - 1];
    }
    return s;
}

fn normalizeHost(raw: []const u8): []const u8 {
    let h = trimAscii(raw);
    if (h.len >= 2 && h[0] == '[' && h[h.len - 1] == ']') {
        h = h[1..h.len - 1];
    }
    return stripTrailingDot(h);
}

fn matchRule(entry_raw: []const u8, norm_host: []const u8, host_ipv4: Ipv4Addr?, port: u16): bool {
    const entry = trimAscii(entry_raw);
    if (entry.len == 0) {
        return false;
    }
    if (entry.len == 1 && entry[0] == '*') {
        return true;
    }

    let rule_host: []const u8 = entry;
    let rule_port: u16? = null;

    // 1. 解析 IPv6 方括号形式（[::1] 或 [::1]:8443）
    if (entry[0] == '[') {
        const close_idx = findByte(entry, ']') ?? return false;
        rule_host = entry[1..close_idx];
        if (close_idx + 1 < entry.len) {
            if (entry[close_idx + 1] != ':') {
                return false;
            }
            const parsed = parsePort(entry[close_idx + 2..]) ?? return false;
            rule_port = parsed;
        }
    } else if (findByte(entry, '/') == null && countByte(entry, ':') == 1) {
        // 2. 单个冒号且非 CIDR：拆分为 host:port（裸 IPv6 含多个冒号不拆分）
        if (colon_idx := findByte(entry, ':')) {
            rule_host = entry[0..colon_idx];
            const parsed = parsePort(entry[colon_idx + 1..]) ?? return false;
            rule_port = parsed;
        }
    }

    // 3. 端口约束检查
    if (rp := rule_port) {
        if (rp != port) {
            return false;
        }
    }

    // 4. IPv4 CIDR 网段匹配
    if (findByte(rule_host, '/') != null) {
        const cidr = Ipv4Cidr.parse(rule_host) ?? return false;
        if (ip := host_ipv4) {
            return cidr.contains(ip);
        }
        return false;
    }

    rule_host = stripTrailingDot(rule_host);
    if (rule_host.len == 0) {
        return false;
    }
    if (rule_host.len == 1 && rule_host[0] == '*') {
        return true;
    }

    // 5. 剥离前缀通配符（*.example.com 或 .example.com）
    let domain = rule_host;
    let wildcard_prefix = false;
    if (domain.len >= 2 && domain[0] == '*' && domain[1] == '.') {
        domain = domain[2..];
        wildcard_prefix = true;
    } else if (domain[0] == '.') {
        domain = domain[1..];
        wildcard_prefix = true;
    }
    if (domain.len == 0) {
        return false;
    }

    // 6. 精确匹配（大小写无关）
    if (asciiEqlIgnoreCase(norm_host, domain)) {
        return true;
    }

    // 7. 域名后缀匹配（仅对非 IPv4 主机名生效，并强制点边界防止 badexample.com 误匹配）
    if (host_ipv4 == null && norm_host.len > domain.len) {
        const dot_pos = norm_host.len - domain.len - 1;
        if (norm_host[dot_pos] == '.' && asciiEqlIgnoreCase(norm_host[dot_pos + 1..], domain)) {
            return true;
        }
    }
    * = wildcard_prefix;
    return false;
}

/// 判断给定 host 与 port 在 NO_PROXY 环境变量规则下是否应绕过 HTTP 代理
export fn shouldBypassProxy(no_proxy_env: []const u8, host: []const u8, port: u16): bool {
    const env = trimAscii(no_proxy_env);
    if (env.len == 0) {
        return false;
    }
    const norm_host = normalizeHost(host);
    if (norm_host.len == 0) {
        return false;
    }
    const host_ipv4 = Ipv4Addr.parse(norm_host);

    let start: usize = 0;
    let i: usize = 0;
    while (i <= env.len) {
        if (i == env.len || env[i] == ',') {
            if (matchRule(env[start..i], norm_host, host_ipv4, port)) {
                return true;
            }
            start = i + 1;
        }
        i += 1;
    }
    return false;
}
