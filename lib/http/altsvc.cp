// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 7838 HTTP Alt-Svc Header Parser & Formatter)

const DEFAULT_ALT_SVC_MAX_AGE: u64 = 86400;
const U64_MAX: u64 = 18446744073709551615;

type AltSvcVec16 = @Vector(16, u8);

fn altsvcSimdFindByte(s: []const u8, start: usize, target: u8): usize? {
    let i: usize = start;
    const n = s.len;
    const v_t: AltSvcVec16 = @splat(target);
    while (i + 16 <= n) {
        const chunk: AltSvcVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_t);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (s[i] == target) return i;
    }
    return null;
}

fn altsvcSimdFindAny2(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i: usize = start;
    const n = s.len;
    const va: AltSvcVec16 = @splat(a);
    const vb: AltSvcVec16 = @splat(b);
    while (i + 16 <= n) {
        const chunk: AltSvcVec16 = s[i..][0..16].*;
        const ma: u16 = @bitCast(chunk == va);
        const mb: u16 = @bitCast(chunk == vb);
        const mask: u16 = ma | mb;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = s[i];
        if (c == a || c == b) return i;
    }
    return n;
}

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
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
    }
    pos.* += s.len;
    return true;
}

fn writeU64Dec(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) {
        return writeSlice(out, pos, "0");
    }
    let tmp: [20]u8 = undefined;
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
    const eq_pos = altsvcSimdFindByte(seg, 0, '=') ?? return null;

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
        i = altsvcSimdFindAny2(seg, i, '"', '\\');
        if (i >= seg.len) break;
        const c = seg[i];
        if (c == '\\') {
            if (i + 1 >= seg.len) {
                return null;
            }
            i += 2;
            continue;
        }
        found_quote_end = true;
        break;
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
        const close_br = altsvcSimdFindByte(auth, 1, ']') ?? return null;
        if (close_br + 1 >= auth.len || auth[close_br + 1] != ':') {
            return null;
        }
        host = auth[0..(close_br + 1)];
        port_str = auth[(close_br + 2)..];
    } else {
        const colon = altsvcSimdFindByte(auth, 0, ':') ?? return null;
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
            if (p_in_quote) {
                if (p_escaped) {
                    p_escaped = false;
                    i += 1;
                    continue;
                }
                i = altsvcSimdFindAny2(seg, i, '"', '\\');
                if (i >= seg.len) break;
                if (seg[i] == '\\') {
                    p_escaped = true;
                } else {
                    p_in_quote = false;
                }
                i += 1;
            } else {
                i = altsvcSimdFindAny2(seg, i, '"', ';');
                if (i >= seg.len) break;
                if (seg[i] == '"') {
                    p_in_quote = true;
                    i += 1;
                } else {
                    break;
                }
            }
        }
        if (p_in_quote) {
            return null;
        }

        const param = trimAscii(seg[p_start..i]);
        if (param.len == 0) {
            continue;
        }

        const peq = altsvcSimdFindByte(param, 0, '=') ?? continue;

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
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                        _.pos += 1;
                        continue;
                    }
                    _.pos = altsvcSimdFindAny2(_.src, _.pos, '"', '\\');
                    if (_.pos >= _.src.len) break;
                    if (_.src[_.pos] == '\\') {
                        escaped = true;
                    } else {
                        in_quote = false;
                    }
                    _.pos += 1;
                } else {
                    _.pos = altsvcSimdFindAny2(_.src, _.pos, '"', ',');
                    if (_.pos >= _.src.len) break;
                    if (_.src[_.pos] == '"') {
                        in_quote = true;
                        _.pos += 1;
                    } else {
                        break;
                    }
                }
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
