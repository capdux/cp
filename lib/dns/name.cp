// Source: Absorbed from /data/dev/bun/src/dns.zig | Synced: 2026-11-21T16:36:24Z (RFC 1035 DNS Name Validator & Wire Label Codec)

type DnsNameVec16 = @Vector(16, u8);

fn makeDnsLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeDnsLabelLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
        lut[i] = if (ok) 1 else 0;
    }
    return lut;
}

const DNS_LOWER_LUT: [u8; 256] = makeDnsLowerLut();
const DNS_LABEL_LUT: [u8; 256] = makeDnsLabelLut();

fn dnsFindDot(s: []const u8, start: usize): usize {
    let i: usize = start;
    const v_dot: DnsNameVec16 = @splat('.' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: DnsNameVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_dot)) break;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') return i;
    }
    return s.len;
}

fn copyLowerAscii(dst: []u8, src: []const u8) {
    let i: usize = 0;
    const lo_A: DnsNameVec16 = @splat(65 as u8);
    const hi_Z: DnsNameVec16 = @splat(90 as u8);
    const case_bit: DnsNameVec16 = @splat(0x20 as u8);
    while (i + 16 <= src.len) : (i += 16) {
        const v: DnsNameVec16 = src[i .. i + 16][0..16].*;
        const lo = @select(u8, (v >= lo_A) & (v <= hi_Z), v | case_bit, v);
        dst[i .. i + 16][0..16].* = lo;
    }
    while (i < src.len) : (i += 1) {
        dst[i] = DNS_LOWER_LUT[@as(usize, src[i])];
    }
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: DnsNameVec16 = @splat(65 as u8);
    const hi_Z: DnsNameVec16 = @splat(90 as u8);
    const case_bit: DnsNameVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: DnsNameVec16 = a[i .. i + 16][0..16].*;
        const vb: DnsNameVec16 = b[i .. i + 16][0..16].*;
        const la = @select(u8, (va >= lo_A) & (va <= hi_Z), va | case_bit, va);
        const lb = @select(u8, (vb >= lo_A) & (vb <= hi_Z), vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (DNS_LOWER_LUT[@as(usize, a[i])] != DNS_LOWER_LUT[@as(usize, b[i])]) {
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
    let i: usize = 0;
    const v_a: DnsNameVec16 = @splat('a' as u8);
    const v_26: DnsNameVec16 = @splat(26 as u8);
    const v_0: DnsNameVec16 = @splat('0' as u8);
    const v_10: DnsNameVec16 = @splat(10 as u8);
    const v_dash: DnsNameVec16 = @splat('-' as u8);
    const v_case: DnsNameVec16 = @splat(0x20 as u8);
    while (i + 16 <= label.len) : (i += 16) {
        const v: DnsNameVec16 = label[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const m_alpha: u16 = @bitCast((vl -% v_a) < v_26);
        const m_dig: u16 = @bitCast((v -% v_0) < v_10);
        const m_dash: u16 = @bitCast(v == v_dash);
        if ((m_alpha | m_dig | m_dash) != 0xFFFF) return false;
    }
    while (i < label.len) : (i += 1) {
        if (DNS_LABEL_LUT[@as(usize, label[i])] == 0) {
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
    while (true) {
        const dot = dnsFindDot(h, start);
        if (!isValidDnsLabel(h[start..dot])) {
            return false;
        }
        if (dot >= h.len) break;
        start = dot + 1;
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
    while (true) {
        const dot = dnsFindDot(h, start);
        const seg = h[start..dot];
        out[pos] = u8(seg.len);
        pos += 1;
        copyLowerAscii(out[pos .. pos + seg.len], seg);
        pos += seg.len;
        if (dot >= h.len) break;
        start = dot + 1;
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
        @memcpy(out[pos .. pos + seg_len], seg);
        pos += seg_len;
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
    copyLowerAscii(out[0..h.len], h);
    return out[0..h.len];
}
