// Source: Absorbed from /data/dev/bun/src/http/http.cp (HAProxy PROXY Protocol v1 Parser & Formatter)

const MAX_PROXY_V1_LEN: usize = 107;

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

fn parsePortStrict(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    // HAProxy v1 严禁端口前导零（"0" 合法，"00"/"080" 非法）
    if (s.len > 1 && s[0] == '0') {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u32(c - '0');
        if (acc > 65535) {
            return null;
        }
        i += 1;
    }
    return acc as u16;
}

fn isValidIpv4(s: []const u8): bool {
    if (s.len < 7 || s.len > 15) {
        return false;
    }
    let parts: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (parts >= 4) {
            return false;
        }
        const seg_start = i;
        while (i < s.len && s[i] != '.') {
            i += 1;
        }
        const seg_len = i - seg_start;
        if (seg_len == 0 || seg_len > 3) {
            return false;
        }
        if (seg_len > 1 && s[seg_start] == '0') {
            return false;
        }
        let val: u32 = 0;
        let k: usize = seg_start;
        while (k < i) {
            const c = s[k];
            if (c < '0' || c > '9') {
                return false;
            }
            val = val * 10 + u32(c - '0');
            k += 1;
        }
        if (val > 255) {
            return false;
        }
        parts += 1;
        if (i < s.len) {
            // 跳过 '.'，不能以 '.' 结尾
            i += 1;
            if (i >= s.len) {
                return false;
            }
        }
    }
    return parts == 4;
}

fn isValidIpv6(s: []const u8): bool {
    if (s.len < 2 || s.len > 39) {
        return false;
    }
    let i: usize = 0;
    let groups: usize = 0;
    let has_double_colon = false;

    if (s[0] == ':') {
        if (s.len < 2 || s[1] != ':') {
            return false;
        }
        has_double_colon = true;
        i = 2;
        if (i == s.len) {
            // "::"
            return true;
        }
    }

    while (i < s.len) {
        const seg_start = i;
        let has_dot = false;
        while (i < s.len && s[i] != ':') {
            if (s[i] == '.') {
                has_dot = true;
            }
            i += 1;
        }
        const seg = s[seg_start..i];
        if (seg.len == 0) {
            return false;
        }

        if (has_dot) {
            // 嵌入 IPv4 必须位于末尾且占 2 个 16-bit 组
            if (i != s.len || !isValidIpv4(seg)) {
                return false;
            }
            groups += 2;
            break;
        }

        if (seg.len > 4) {
            return false;
        }
        let k: usize = 0;
        while (k < seg.len) {
            if (!isHexDigit(seg[k])) {
                return false;
            }
            k += 1;
        }
        groups += 1;
        if (groups > 8) {
            return false;
        }

        if (i < s.len) {
            // 当前为 ':'
            i += 1;
            if (i >= s.len) {
                // 不能以单个 ':' 结尾
                return false;
            }
            if (s[i] == ':') {
                if (has_double_colon) {
                    return false;
                }
                has_double_colon = true;
                i += 1;
                if (i == s.len) {
                    // 以 "::" 结尾合法
                    break;
                }
            }
        }
    }

    if (has_double_colon) {
        return groups < 8;
    }
    return groups == 8;
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeU16Dec(out: []u8, pos: *usize, val: u16): bool {
    if (val == 0) {
        return writeByte(out, pos, '0');
    }
    let tmp: [5]u8 = [0; 5];
    let n: usize = 0;
    let v: u32 = val;
    while (v > 0) {
        tmp[n] = ((v % 10) as u8) + '0';
        n += 1;
        v /= 10;
    }
    if (out.len - pos.* < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

/// HAProxy PROXY Protocol v1 传输协议族
export enum ProxyProtoFamily {
    tcp4,
    tcp6,
    unknown,
}

/// HAProxy PROXY Protocol v1 解析结果（零堆分配切片视图）
export struct ProxyV1Header {
    family: ProxyProtoFamily;
    src_addr: []const u8;
    dst_addr: []const u8;
    src_port: u16;
    dst_port: u16;
    header_len: usize;

    /// 严格按 HAProxy PROXY v1 规范解析头部（最大 107 字节，以 `"PROXY "` 开头与 `\r\n` 结尾）
    fn parse(buf: []const u8): ProxyV1Header? {
        // 最短合法头为 "PROXY UNKNOWN\r\n"（15 字节）
        if (buf.len < 15) {
            return null;
        }
        if (!eqlBytes(buf[0..6], "PROXY ")) {
            return null;
        }

        // 在前 107 字节内查找首个 "\r\n"，并禁止提前出现孤立 '\r' 或 '\n'
        const scan_limit: usize = if (buf.len < MAX_PROXY_V1_LEN) buf.len else MAX_PROXY_V1_LEN;
        let line_end: usize = 0;
        let found_crlf = false;
        let i: usize = 6;
        while (i < scan_limit) {
            if (buf[i] == '\r') {
                if (i + 1 < scan_limit && buf[i + 1] == '\n') {
                    line_end = i;
                    found_crlf = true;
                    break;
                }
                return null;
            }
            if (buf[i] == '\n') {
                return null;
            }
            i += 1;
        }
        if (!found_crlf) {
            return null;
        }

        const header_len = line_end + 2;
        const body = buf[6..line_end];

        // 提取协议族字段
        let p_end: usize = 0;
        while (p_end < body.len && body[p_end] != ' ') {
            p_end += 1;
        }
        const proto = body[0..p_end];

        if (eqlBytes(proto, "UNKNOWN")) {
            // "UNKNOWN" 后要么结束，要么跟单空格及任意附加信息直至 \r\n
            return ProxyV1Header{
                family: ProxyProtoFamily.unknown,
                src_addr: "",
                dst_addr: "",
                src_port: 0,
                dst_port: 0,
                header_len: header_len,
            };
        }

        let family = ProxyProtoFamily.tcp4;
        if (eqlBytes(proto, "TCP4")) {
            family = ProxyProtoFamily.tcp4;
        } else if (eqlBytes(proto, "TCP6")) {
            family = ProxyProtoFamily.tcp6;
        } else {
            return null;
        }

        // 严格单空格分隔 4 个剩余字段：src_addr dst_addr src_port dst_port
        if (p_end >= body.len || body[p_end] != ' ') {
            return null;
        }
        let cursor = p_end + 1;

        // 1. src_addr
        const src_start = cursor;
        while (cursor < body.len && body[cursor] != ' ') {
            cursor += 1;
        }
        const src_addr = body[src_start..cursor];
        if (src_addr.len == 0 || cursor >= body.len) {
            return null;
        }
        cursor += 1;

        // 2. dst_addr
        const dst_start = cursor;
        while (cursor < body.len && body[cursor] != ' ') {
            cursor += 1;
        }
        const dst_addr = body[dst_start..cursor];
        if (dst_addr.len == 0 || cursor >= body.len) {
            return null;
        }
        cursor += 1;

        // 3. src_port
        const sp_start = cursor;
        while (cursor < body.len && body[cursor] != ' ') {
            cursor += 1;
        }
        const sp_str = body[sp_start..cursor];
        if (sp_str.len == 0 || cursor >= body.len) {
            return null;
        }
        cursor += 1;

        // 4. dst_port（必须直达行尾，不能包含额外空格）
        const dp_start = cursor;
        while (cursor < body.len && body[cursor] != ' ') {
            cursor += 1;
        }
        if (cursor != body.len) {
            return null;
        }
        const dp_str = body[dp_start..cursor];
        if (dp_str.len == 0) {
            return null;
        }

        if (family == ProxyProtoFamily.tcp4) {
            if (!isValidIpv4(src_addr) || !isValidIpv4(dst_addr)) {
                return null;
            }
        } else {
            if (!isValidIpv6(src_addr) || !isValidIpv6(dst_addr)) {
                return null;
            }
        }

        const src_port = parsePortStrict(sp_str) ?? return null;
        const dst_port = parsePortStrict(dp_str) ?? return null;

        return ProxyV1Header{
            family: family,
            src_addr: src_addr,
            dst_addr: dst_addr,
            src_port: src_port,
            dst_port: dst_port,
            header_len: header_len,
        };
    }

    /// 规范化输出 HAProxy PROXY v1 头部（`PROXY TCP4/TCP6 ... \r\n` 或 `PROXY UNKNOWN\r\n`）
    fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;

        if (_.family == ProxyProtoFamily.unknown) {
            if (!writeSlice(out, &pos, "PROXY UNKNOWN\r\n")) {
                return null;
            }
            return out[0..pos];
        }

        if (_.family == ProxyProtoFamily.tcp4) {
            if (!isValidIpv4(_.src_addr) || !isValidIpv4(_.dst_addr)) {
                return null;
            }
            if (!writeSlice(out, &pos, "PROXY TCP4 ")) {
                return null;
            }
        } else {
            if (!isValidIpv6(_.src_addr) || !isValidIpv6(_.dst_addr)) {
                return null;
            }
            if (!writeSlice(out, &pos, "PROXY TCP6 ")) {
                return null;
            }
        }

        if (!writeSlice(out, &pos, _.src_addr)) {
            return null;
        }
        if (!writeByte(out, &pos, ' ')) {
            return null;
        }
        if (!writeSlice(out, &pos, _.dst_addr)) {
            return null;
        }
        if (!writeByte(out, &pos, ' ')) {
            return null;
        }
        if (!writeU16Dec(out, &pos, _.src_port)) {
            return null;
        }
        if (!writeByte(out, &pos, ' ')) {
            return null;
        }
        if (!writeU16Dec(out, &pos, _.dst_port)) {
            return null;
        }
        if (!writeSlice(out, &pos, "\r\n")) {
            return null;
        }

        if (pos > MAX_PROXY_V1_LEN) {
            return null;
        }
        return out[0..pos];
    }
}
