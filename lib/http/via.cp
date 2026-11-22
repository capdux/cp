// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (RFC 9110 Via Proxy Chain & Max-Forwards)

const U32_MAX: u32 = 0xFFFF_FFFF;

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

/// RFC 9110 §7.6.3 单条 Via 代理跳信息（零拷贝切片）
export struct ViaEntry {
    protocol_name: ([]const u8)?;
    protocol_version: []const u8;
    received_by: []const u8;
    comment: ([]const u8)?;

    /// 返回生效协议名（省略 protocol_name 时按 RFC 9110 默认为 "HTTP"）
    fn effectiveProtocolName(): []const u8 {
        if (pn := _.protocol_name) {
            return pn;
        }
        return "HTTP";
    }
}

/// RFC 9110 `Via` 响应/请求头流式迭代器（零堆分配，支持括号注释内含逗号与嵌套）
export struct ViaIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ViaIter {
        return ViaIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ViaEntry? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            let paren_depth: usize = 0;
            let escaped = false;

            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (paren_depth > 0) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '(') {
                        paren_depth += 1;
                    } else if (c == ')') {
                        paren_depth -= 1;
                    }
                } else {
                    if (c == '(') {
                        paren_depth += 1;
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
            if (seg.len == 0) {
                continue;
            }

            // 1. 提取 received-protocol
            let i: usize = 0;
            while (i < seg.len && !isSpace(seg[i])) {
                i += 1;
            }
            const proto_tok = seg[0..i];
            if (proto_tok.len == 0) {
                continue;
            }

            let protocol_name: ([]const u8)? = null;
            let protocol_version: []const u8 = proto_tok;
            if (slash := findByte(proto_tok, '/')) {
                const pname = proto_tok[0..slash];
                const pver = proto_tok[slash + 1..];
                if (pname.len == 0 || pver.len == 0) {
                    continue;
                }
                protocol_name = pname;
                protocol_version = pver;
            }

            // 2. 跳过空白并提取 received-by
            while (i < seg.len && isSpace(seg[i])) {
                i += 1;
            }
            if (i >= seg.len || seg[i] == '(') {
                continue;
            }
            const rb_start = i;
            while (i < seg.len && !isSpace(seg[i]) && seg[i] != '(') {
                i += 1;
            }
            const received_by = seg[rb_start..i];
            if (received_by.len == 0) {
                continue;
            }

            // 3. 可选 (...) 注释
            let comment: ([]const u8)? = null;
            const rest = trimAscii(seg[i..]);
            if (rest.len >= 2 && rest[0] == '(' && rest[rest.len - 1] == ')') {
                comment = rest[1..rest.len - 1];
            }

            return ViaEntry{
                protocol_name: protocol_name,
                protocol_version: protocol_version,
                received_by: received_by,
                comment: comment,
            };
        }
        return null;
    }
}

/// 统计 Via 头部中包含的代理跳数
export fn viaHopCount(header: []const u8): usize {
    let it = ViaIter.init(header);
    let count: usize = 0;
    while (it.next() != null) {
        count += 1;
    }
    return count;
}

/// 大小写不敏感检测 Via 代理链中是否已出现指定代理节点（用于代理环路检测）
export fn viaContainsProxy(header: []const u8, received_by: []const u8): bool {
    const target = trimAscii(received_by);
    if (target.len == 0) {
        return false;
    }
    let it = ViaIter.init(header);
    while (true) {
        if (entry := it.next()) {
            if (asciiEqlIgnoreCase(entry.received_by, target)) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

fn copySlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.*] = s[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// 向现有 Via 头部追加当前代理节点（零堆分配）
export fn appendVia(existing: []const u8, protocol_version: []const u8, received_by: []const u8, out: []u8): ([]const u8)? {
    const pv = trimAscii(protocol_version);
    const rb = trimAscii(received_by);
    if (pv.len == 0 || rb.len == 0) {
        return null;
    }

    const base = trimAscii(existing);
    let pos: usize = 0;
    if (base.len > 0) {
        if (!copySlice(out, &pos, base)) return null;
        if (!copySlice(out, &pos, ", ")) return null;
    }
    if (!copySlice(out, &pos, pv)) return null;
    if (!copySlice(out, &pos, " ")) return null;
    if (!copySlice(out, &pos, rb)) return null;
    return out[0..pos];
}

/// 严格解析 RFC 9110 §7.6.2 `Max-Forwards` 非负整数头部（防 u32 溢出）
export fn parseMaxForwards(header: []const u8): u32? {
    const s = trimAscii(header);
    if (s.len == 0) {
        return null;
    }
    let val: u32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        const d: u32 = u32(c - '0');
        if (val > (U32_MAX - d) / 10) {
            return null;
        }
        val = val * 10 + d;
    }
    return val;
}

/// 递减 Max-Forwards 计数；当值为 0 时返回 null 表示必须停止转发并直接响应
export fn decrementMaxForwards(val: u32): u32? {
    if (val == 0) {
        return null;
    }
    return val - 1;
}
