// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T11:12:13Z (W3C Trace Context traceparent / tracestate)

const HEX_LOWER: []const u8 = "0123456789abcdef";

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn lowerHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return null;
}

fn parseLowerHexByte(s: []const u8, offset: usize): u8? {
    const hi = lowerHexNibble(s[offset]) ?? return null;
    const lo = lowerHexNibble(s[offset + 1]) ?? return null;
    return (hi << 4) | lo;
}

fn writeHexByte(out: []u8, pos: usize, b: u8) {
    out[pos] = HEX_LOWER[usize(b >> 4)];
    out[pos + 1] = HEX_LOWER[usize(b & 0x0F)];
}

fn strEql(a: []const u8, b: []const u8): bool {
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

/// W3C Trace Context Level 1 `traceparent` 头部结构（零堆分配）
export struct TraceParent {
    version: u8;
    trace_id: [16]u8;
    parent_id: [8]u8;
    trace_flags: u8;

    /// 严格解析 W3C `traceparent` 头部
    /// 拒绝大写十六进制、version == 0xff、全零 trace_id 或全零 parent_id；支持 version > 0 后带 `-extra` 向前兼容
    fn parse(header: []const u8): TraceParent? {
        const s = trimOws(header);
        if (s.len < 55) {
            return null;
        }
        if (s[2] != '-' || s[35] != '-' || s[52] != '-') {
            return null;
        }

        const version = parseLowerHexByte(s, 0) ?? return null;
        if (version == 0xFF) {
            return null;
        }
        if (version == 0x00) {
            if (s.len != 55) {
                return null;
            }
        } else {
            if (s.len > 55 && s[55] != '-') {
                return null;
            }
        }

        let trace_id: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let trace_nonzero = false;
        let i: usize = 0;
        while (i < 16) {
            const b = parseLowerHexByte(s, 3 + i * 2) ?? return null;
            trace_id[i] = b;
            if (b != 0) {
                trace_nonzero = true;
            }
            i += 1;
        }
        if (!trace_nonzero) {
            return null;
        }

        let parent_id: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
        let parent_nonzero = false;
        let j: usize = 0;
        while (j < 8) {
            const b = parseLowerHexByte(s, 36 + j * 2) ?? return null;
            parent_id[j] = b;
            if (b != 0) {
                parent_nonzero = true;
            }
            j += 1;
        }
        if (!parent_nonzero) {
            return null;
        }

        const flags = parseLowerHexByte(s, 53) ?? return null;

        return TraceParent{
            version: version,
            trace_id: trace_id,
            parent_id: parent_id,
            trace_flags: flags,
        };
    }

    /// 是否设置了 sampled 采样标志位（bit 0）
    fn isSampled(): bool {
        return (_.trace_flags & 0x01) != 0;
    }

    /// 返回更新采样标志位后的新 TraceParent 副本
    fn withSampled(sampled: bool): TraceParent {
        const next_flags: u8 = if (sampled) (_.trace_flags | u8(0x01)) else (_.trace_flags & ~u8(0x01));
        return TraceParent{
            version: _.version,
            trace_id: _.trace_id,
            parent_id: _.parent_id,
            trace_flags: next_flags,
        };
    }

    /// 返回替换 parent_id（子 Span ID）后的新 TraceParent 副本
    fn withParentId(new_parent: [8]u8): TraceParent {
        return TraceParent{
            version: _.version,
            trace_id: _.trace_id,
            parent_id: new_parent,
            trace_flags: _.trace_flags,
        };
    }

    /// 规范化格式化为 55 字节小写 `traceparent` 头部字符串（零堆分配）
    fn format(out: []u8): ([]const u8)? {
        if (out.len < 55) {
            return null;
        }
        writeHexByte(out, 0, _.version);
        out[2] = '-';

        let i: usize = 0;
        while (i < 16) {
            writeHexByte(out, 3 + i * 2, _.trace_id[i]);
            i += 1;
        }
        out[35] = '-';

        let j: usize = 0;
        while (j < 8) {
            writeHexByte(out, 36 + j * 2, _.parent_id[j]);
            j += 1;
        }
        out[52] = '-';
        writeHexByte(out, 53, _.trace_flags);
        return out[0..55];
    }
}

fn isValidSimpleKeyChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '_'
        || c == '-'
        || c == '*'
        || c == '/';
}

fn isValidTraceStateKey(key: []const u8): bool {
    if (key.len == 0 || key.len > 256) {
        return false;
    }
    if (at_idx := findByte(key, '@')) {
        const tenant = key[0..at_idx];
        const sys = key[at_idx + 1..];
        if (tenant.len == 0 || tenant.len > 241 || sys.len == 0 || sys.len > 14) {
            return false;
        }
        const t0 = tenant[0];
        if (!((t0 >= 'a' && t0 <= 'z') || (t0 >= '0' && t0 <= '9'))) {
            return false;
        }
        for (c of tenant) {
            if (!isValidSimpleKeyChar(c)) return false;
        }
        const s0 = sys[0];
        if (!(s0 >= 'a' && s0 <= 'z')) {
            return false;
        }
        for (c of sys) {
            if (!isValidSimpleKeyChar(c)) return false;
        }
        return true;
    }

    const k0 = key[0];
    if (!(k0 >= 'a' && k0 <= 'z')) {
        return false;
    }
    for (c of key) {
        if (!isValidSimpleKeyChar(c)) return false;
    }
    return true;
}

fn isValidTraceStateValue(val: []const u8): bool {
    if (val.len == 0 || val.len > 256) {
        return false;
    }
    if (val[val.len - 1] == ' ') {
        return false;
    }
    for (c of val) {
        if (c < 0x20 || c > 0x7E || c == ',' || c == '=') {
            return false;
        }
    }
    return true;
}

/// W3C `tracestate` 单个 vendor `key=value` 成员（零拷贝切片）
export struct TraceStateEntry {
    key: []const u8;
    value: []const u8;
}

/// W3C `tracestate` 逗号分隔成员流式迭代器
export struct TraceStateIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): TraceStateIter {
        return TraceStateIter{
            src: header,
            pos: 0,
        };
    }

    /// 迭代下一个合法的 `key=value` 成员；自动跳过空成员与畸形项
    fn next(*_): TraceStateEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = trimOws(_.src[start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }
            if (eq_idx := findByte(seg, '=')) {
                const key = seg[0..eq_idx];
                const val = seg[eq_idx + 1..];
                if (isValidTraceStateKey(key) && isValidTraceStateValue(val)) {
                    return TraceStateEntry{
                        key: key,
                        value: val,
                    };
                }
            }
        }
        return null;
    }
}

/// 在 `tracestate` 头部中精确查找指定 vendor key 对应的值
export fn findTraceState(header: []const u8, key: []const u8): ([]const u8)? {
    let it = TraceStateIter.init(header);
    while (true) {
        if (entry := it.next()) {
            if (strEql(entry.key, key)) {
                return entry.value;
            }
        } else {
            break;
        }
    }
    return null;
}
