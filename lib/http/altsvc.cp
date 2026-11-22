// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (RFC 7838 HTTP Alt-Svc Header Parser & Formatter)

const DEFAULT_ALT_SVC_MAX_AGE: u64 = 86400;
const U64_MAX: u64 = 18446744073709551615;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isCtrlOrSpace(c: u8): bool {
    return c <= 0x20 || c == 0x7F;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn isValidProtoId(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCtrlOrSpace(c) || c == '=' || c == '"' || c == ';' || c == ',') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidHostSlice(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCtrlOrSpace(c) || c == '"' || c == ';' || c == ',') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parsePortNonZero(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
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
    if (acc == 0) {
        return null;
    }
    return acc as u16;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (acc > (U64_MAX - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn unquoteIfQuoted(s: []const u8): ([]const u8)? {
    if (s.len == 0 || s[0] != '"') {
        return s;
    }
    if (s.len < 2 || s[s.len - 1] != '"') {
        return null;
    }
    return s[1..(s.len - 1)];
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

fn writeU64Dec(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) {
        return writeSlice(out, pos, "0");
    }
    let tmp: [20]u8 = [0; 20];
    let n: usize = 0;
    let v = val;
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

/// RFC 7838 单条 `Alt-Svc` 备选服务条目（零堆分配切片视图）
export struct AltSvcEntry {
    protocol_id: []const u8;
    host: []const u8;
    port: u16;
    max_age: u64;
    persist: bool;

    /// 判断协议是否为 HTTP/3（`"h3"` 或 `"h3-*"` 草案版本）
    fn isHttp3(): bool {
        if (eqlBytes(_.protocol_id, "h3")) {
            return true;
        }
        return _.protocol_id.len >= 3
            && _.protocol_id[0] == 'h'
            && _.protocol_id[1] == '3'
            && _.protocol_id[2] == '-';
    }
}

/// 判断 `Alt-Svc` 头部是否为清除指令 `"clear"`（大小写不敏感）
export fn isAltSvcClear(header: []const u8): bool {
    return eqlIgnoreCase(trimAscii(header), "clear");
}

fn parseSingleAltSvc(seg: []const u8): AltSvcEntry? {
    // 1. protocol_id = "<alt-authority>"
    let eq_pos: usize = 0;
    let found_eq = false;
    while (eq_pos < seg.len) {
        if (seg[eq_pos] == '=') {
            found_eq = true;
            break;
        }
        eq_pos += 1;
    }
    if (!found_eq) {
        return null;
    }

    const proto = trimAscii(seg[0..eq_pos]);
    if (!isValidProtoId(proto)) {
        return null;
    }

    let i: usize = eq_pos + 1;
    while (i < seg.len && isSpace(seg[i])) {
        i += 1;
    }
    if (i >= seg.len || seg[i] != '"') {
        return null;
    }

    // 2. 提取带引号的 alt-authority
    i += 1;
    const auth_start = i;
    let found_quote_end = false;
    while (i < seg.len) {
        const c = seg[i];
        if (c == '\\') {
            if (i + 1 >= seg.len) {
                return null;
            }
            i += 2;
            continue;
        }
        if (c == '"') {
            found_quote_end = true;
            break;
        }
        i += 1;
    }
    if (!found_quote_end) {
        return null;
    }

    const auth = seg[auth_start..i];
    i += 1;

    // 3. 解析 [host]:port / host:port / :port
    let host: []const u8 = "";
    let port_str: []const u8 = "";

    if (auth.len < 2) {
        return null;
    }
    if (auth[0] == '[') {
        let close_br: usize = 1;
        let found_br = false;
        while (close_br < auth.len) {
            if (auth[close_br] == ']') {
                found_br = true;
                break;
            }
            close_br += 1;
        }
        if (!found_br || close_br + 1 >= auth.len || auth[close_br + 1] != ':') {
            return null;
        }
        host = auth[0..(close_br + 1)];
        port_str = auth[(close_br + 2)..];
    } else {
        let colon: usize = 0;
        let found_colon = false;
        while (colon < auth.len) {
            if (auth[colon] == ':') {
                found_colon = true;
                break;
            }
            colon += 1;
        }
        if (!found_colon) {
            return null;
        }
        host = auth[0..colon];
        port_str = auth[(colon + 1)..];
    }

    if (!isValidHostSlice(host)) {
        return null;
    }
    const port = parsePortNonZero(port_str) ?? return null;

    // 4. 解析可选分号参数（ma / persist）
    let max_age: u64 = DEFAULT_ALT_SVC_MAX_AGE;
    let persist = false;

    while (i < seg.len && isSpace(seg[i])) {
        i += 1;
    }
    if (i < seg.len && seg[i] != ';') {
        return null;
    }

    while (i < seg.len) {
        if (seg[i] == ';') {
            i += 1;
        }
        const p_start = i;
        let p_in_quote = false;
        let p_escaped = false;
        while (i < seg.len) {
            const c = seg[i];
            if (p_in_quote) {
                if (p_escaped) {
                    p_escaped = false;
                } else if (c == '\\') {
                    p_escaped = true;
                } else if (c == '"') {
                    p_in_quote = false;
                }
            } else {
                if (c == '"') {
                    p_in_quote = true;
                } else if (c == ';') {
                    break;
                }
            }
            i += 1;
        }
        if (p_in_quote) {
            return null;
        }

        const param = trimAscii(seg[p_start..i]);
        if (param.len == 0) {
            continue;
        }

        let peq: usize = 0;
        let has_peq = false;
        while (peq < param.len) {
            if (param[peq] == '=') {
                has_peq = true;
                break;
            }
            peq += 1;
        }
        if (!has_peq) {
            continue;
        }

        const pkey = trimAscii(param[0..peq]);
        const pval_raw = trimAscii(param[(peq + 1)..]);
        const pval = unquoteIfQuoted(pval_raw) ?? return null;

        if (eqlIgnoreCase(pkey, "ma")) {
            if (v := parseU64Dec(pval)) {
                max_age = v;
            }
        } else if (eqlIgnoreCase(pkey, "persist")) {
            if (eqlBytes(pval, "1")) {
                persist = true;
            }
        }
    }

    return AltSvcEntry{
        protocol_id: proto,
        host: host,
        port: port,
        max_age: max_age,
        persist: persist,
    };
}

/// RFC 7838 `Alt-Svc` 头部多条目流式迭代器（引号感知，自动跳过 `clear` 与畸形项）
export struct AltSvcIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): AltSvcIter {
        if (isAltSvcClear(header)) {
            return AltSvcIter{
                src: header,
                pos: header.len,
            };
        }
        return AltSvcIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): AltSvcEntry? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let in_quote = false;
            let escaped = false;

            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }

            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0 || in_quote) {
                continue;
            }
            if (entry := parseSingleAltSvc(seg)) {
                return entry;
            }
        }
        return null;
    }
}

/// 规范化输出单条 RFC 7838 `Alt-Svc` 条目（端口为 0、字段非法或缓冲不足返回 `null`）
export fn formatAltSvcEntry(entry: AltSvcEntry, out: []u8): ([]const u8)? {
    if (!isValidProtoId(entry.protocol_id) || !isValidHostSlice(entry.host) || entry.port == 0) {
        return null;
    }

    let pos: usize = 0;
    if (!writeSlice(out, &pos, entry.protocol_id)) return null;
    if (!writeSlice(out, &pos, "=\"")) return null;
    if (!writeSlice(out, &pos, entry.host)) return null;
    if (!writeSlice(out, &pos, ":")) return null;
    if (!writeU64Dec(out, &pos, u64(entry.port))) return null;
    if (!writeSlice(out, &pos, "\"")) return null;

    if (entry.max_age != DEFAULT_ALT_SVC_MAX_AGE) {
        if (!writeSlice(out, &pos, "; ma=")) return null;
        if (!writeU64Dec(out, &pos, entry.max_age)) return null;
    }
    if (entry.persist) {
        if (!writeSlice(out, &pos, "; persist=1")) return null;
    }

    return out[0..pos];
}
