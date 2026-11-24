// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (W3C Trace Context traceparent / tracestate)
// Reference-hardened against open-telemetry/opentelemetry-rust (Apache-2.0)

import {
    TRACESTATE_MAX_ENTRIES,
    simdIsLowerHex,
    simdIsAllZero16,
    simdHasControlByte,
} from "./traceParent.cp";

const W3C_HEX_LOWER: []const u8 = "0123456789abcdef";

fn w3cIsOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn w3cTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && w3cIsOws(s[start])) {
        start += 1;
    }
    while (end > start && w3cIsOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn w3cHasNonTabControlByte(s: []const u8): bool {
    if (!simdHasControlByte(s)) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if ((c < 0x20 && c != '\t') || c == 0x7F) return true;
    }
    return false;
}

fn w3cIsAllZero8(bytes: [8]u8): bool {
    let padded: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        padded[i] = bytes[i];
    }
    return simdIsAllZero16(padded);
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
    out[pos] = W3C_HEX_LOWER[@as(usize, b >> 4)];
    out[pos + 1] = W3C_HEX_LOWER[@as(usize, b & 0x0F)];
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

/// W3C Trace Context Level 1/2 `traceparent` 头部结构（零堆分配）
pub struct W3cTraceParent {
    version: u8;
    trace_id: [16]u8;
    parent_id: [8]u8;
    trace_flags: u8;

    /// 严格解析 W3C `traceparent` 头部
    /// 拒绝控制字符/CRLF、大写十六进制、version == 0xff、全零 trace_id 或全零 parent_id；
    /// 支持 version > 0 后带 `-extra` 向前兼容，热路径启用 16-lane SIMD 加速。
    static fn parse(header: []const u8): W3cTraceParent? {
        if (w3cHasNonTabControlByte(header)) {
            return null;
        }
        const s = w3cTrimOws(header);
        if (s.len < 55 || simdHasControlByte(s)) {
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
        } else if (s.len > 55) {
            if (s[55] != '-' || s.len == 56) {
                return null;
            }
            let ei: usize = 56;
            while (ei < s.len) : (ei += 1) {
                if (s[ei] < 0x20 || s[ei] > 0x7E) {
                    return null;
                }
            }
        }

        // 16-lane SIMD lowercase hex validation for 32B trace-id and 16B parent-id
        if (!simdIsLowerHex(s[3..35]) || !simdIsLowerHex(s[36..52])) {
            return null;
        }

        let trace_id: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 16) {
            const b = parseLowerHexByte(s, 3 + i * 2) ?? return null;
            trace_id[i] = b;
            i += 1;
        }
        if (simdIsAllZero16(trace_id)) {
            return null;
        }

        let parent_id: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
        let j: usize = 0;
        while (j < 8) {
            const b = parseLowerHexByte(s, 36 + j * 2) ?? return null;
            parent_id[j] = b;
            j += 1;
        }
        if (w3cIsAllZero8(parent_id)) {
            return null;
        }

        const flags = parseLowerHexByte(s, 53) ?? return null;

        return W3cTraceParent{
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

    /// 返回更新采样标志位后的新 W3cTraceParent 副本
    fn withSampled(sampled: bool): W3cTraceParent {
        const next_flags: u8 = if (sampled) (_.trace_flags | @as(u8, 0x01)) else (_.trace_flags & ~@as(u8, 0x01));
        return W3cTraceParent{
            version: _.version,
            trace_id: _.trace_id,
            parent_id: _.parent_id,
            trace_flags: next_flags,
        };
    }

    /// 返回替换 parent_id（子 Span ID）后的新 W3cTraceParent 副本
    fn withParentId(new_parent: [8]u8): W3cTraceParent {
        return W3cTraceParent{
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

fn w3cIsValidTraceStateKey(key: []const u8): bool {
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

fn w3cIsValidTraceStateValue(val: []const u8): bool {
    if (val.len == 0 || val.len > 256) {
        return false;
    }
    if (val[val.len - 1] == ' ') {
        return false;
    }
    if (simdHasControlByte(val)) {
        return false;
    }
    for (c of val) {
        if (c > 0x7E || c == ',' || c == '=') {
            return false;
        }
    }
    return true;
}

/// W3C `tracestate` 单个 vendor `key=value` 成员（零拷贝切片）
pub struct W3cTraceStateEntry {
    key: []const u8;
    value: []const u8;
}

/// W3C `tracestate` 逗号分隔成员流式迭代器（严格执行最多 32 个成员与控制字符防护）
pub struct TraceStateIter {
    src: []const u8;
    pos: usize;
    yielded: usize = 0;

    static fn init(header: []const u8): TraceStateIter {
        if (w3cHasNonTabControlByte(header)) {
            return TraceStateIter{
                src: header,
                pos: header.len,
                yielded: TRACESTATE_MAX_ENTRIES,
            };
        }
        return TraceStateIter{
            src: header,
            pos: 0,
            yielded: 0,
        };
    }

    /// 迭代下一个合法的 `key=value` 成员；自动跳过空成员与畸形项，最多返回 32 项
    fn next(*_): W3cTraceStateEntry? {
        if (_.yielded >= TRACESTATE_MAX_ENTRIES) {
            return null;
        }
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = w3cTrimOws(_.src[start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }
            if (eq_idx := findByte(seg, '=')) {
                const key = seg[0..eq_idx];
                const val = seg[eq_idx + 1..];
                if (w3cIsValidTraceStateKey(key) && w3cIsValidTraceStateValue(val)) {
                    _.yielded += 1;
                    return W3cTraceStateEntry{
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
pub fn findTraceState(header: []const u8, key: []const u8): ([]const u8)? {
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
