// Source: Absorbed from /data/dev/bun/src/dns.zig (RFC 1035 DNS Name Validator & Wire Label Codec)

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isAlphaNum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
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

fn stripSingleTrailingDot(s: []const u8): []const u8 {
    if (s.len > 0 && s[s.len - 1] == '.') {
        return s[0..s.len - 1];
    }
    return s;
}

/// 校验单个 RFC 1035 / RFC 1123 DNS 标签（长度 1..=63，仅 `[A-Za-z0-9-]`，首尾不为 `-`）
export fn isValidDnsLabel(label: []const u8): bool {
    if (label.len == 0 || label.len > 63) {
        return false;
    }
    if (label[0] == '-' || label[label.len - 1] == '-') {
        return false;
    }
    for (c of label) {
        if (!isAlphaNum(c) && c != '-') {
            return false;
        }
    }
    return true;
}

/// 校验完整 DNS 主机名（允许末尾可选单个 `.`，去尾点后总长 1..=253，每段满足 isValidDnsLabel）
export fn isValidHostname(host: []const u8): bool {
    const h = stripSingleTrailingDot(host);
    if (h.len == 0 || h.len > 253) {
        return false;
    }

    let start: usize = 0;
    let i: usize = 0;
    while (i <= h.len) {
        if (i == h.len || h[i] == '.') {
            if (!isValidDnsLabel(h[start..i])) {
                return false;
            }
            start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// 将主机名编码为 RFC 1035 长度前缀线缆格式（小写规范化，以 `0x00` 结尾，零堆分配）
export fn encodeWireName(host: []const u8, out: []u8): ([]const u8)? {
    // 根域名 "." 编码为单字节 0x00
    if (host.len == 1 && host[0] == '.') {
        if (out.len < 1) return null;
        out[0] = 0;
        return out[0..1];
    }

    if (!isValidHostname(host)) {
        return null;
    }
    const h = stripSingleTrailingDot(host);
    const needed = h.len + 2;
    if (out.len < needed) {
        return null;
    }

    let pos: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= h.len) {
        if (i == h.len || h[i] == '.') {
            const seg = h[start..i];
            out[pos] = u8(seg.len);
            pos += 1;
            for (c of seg) {
                out[pos] = toLowerAscii(c);
                pos += 1;
            }
            start = i + 1;
        }
        i += 1;
    }

    out[pos] = 0;
    pos += 1;
    return out[0..pos];
}

/// 解码非压缩 RFC 1035 线缆格式回点分域名；遇压缩指针（>= 0xC0）、保留标签位、非法标签或截断返回 null
export fn decodeWireName(wire: []const u8, out: []u8): ([]const u8)? {
    if (wire.len == 0) {
        return null;
    }

    let i: usize = 0;
    let pos: usize = 0;
    let saw_zero = false;

    while (i < wire.len) {
        const len_b = wire[i];
        if (len_b == 0) {
            saw_zero = true;
            break;
        }
        // RFC 1035：长度高两位必须为 00（<= 63）；>= 0xC0 为压缩指针，0x40..0xBF 为保留位
        if (len_b > 63) {
            return null;
        }
        const seg_len: usize = usize(len_b);
        if (i + 1 + seg_len > wire.len) {
            return null;
        }
        const seg = wire[i + 1..i + 1 + seg_len];
        if (!isValidDnsLabel(seg)) {
            return null;
        }

        const extra_dot: usize = if (pos > 0) 1 else 0;
        if (pos + extra_dot + seg_len > 253 || pos + extra_dot + seg_len > out.len) {
            return null;
        }
        if (pos > 0) {
            out[pos] = '.';
            pos += 1;
        }
        for (c of seg) {
            out[pos] = c;
            pos += 1;
        }
        i += 1 + seg_len;
    }

    if (!saw_zero) {
        return null;
    }
    return out[0..pos];
}

/// 判断 host 是否为 parent 的子域（含相等；大小写不敏感、忽略末尾 `.`，严格点边界防误匹配）
export fn isSubdomainOf(host: []const u8, parent: []const u8): bool {
    if (!isValidHostname(host) || !isValidHostname(parent)) {
        return false;
    }
    const h = stripSingleTrailingDot(host);
    const p = stripSingleTrailingDot(parent);

    if (asciiEqlIgnoreCase(h, p)) {
        return true;
    }
    if (h.len > p.len) {
        const dot_idx = h.len - p.len - 1;
        if (h[dot_idx] == '.' && asciiEqlIgnoreCase(h[dot_idx + 1..], p)) {
            return true;
        }
    }
    return false;
}

/// 规范化主机名：校验合法性、小写化并剥离末尾可选 `.`（零堆分配）
export fn normalizeHostname(host: []const u8, out: []u8): ([]const u8)? {
    if (!isValidHostname(host)) {
        return null;
    }
    const h = stripSingleTrailingDot(host);
    if (out.len < h.len) {
        return null;
    }
    let i: usize = 0;
    while (i < h.len) {
        out[i] = toLowerAscii(h[i]);
        i += 1;
    }
    return out[0..h.len];
}
