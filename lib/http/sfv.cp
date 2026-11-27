// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 8941 HTTP Structured Field Values)

/// RFC 8941 HTTP Structured Field Values 标量项类型
export enum SfItemKind {
    integer,
    decimal_milli,
    string,
    token,
    byte_seq,
    boolean,
}

/// RFC 8941 标量项（零拷贝切片 + 纯整数千分小数表示）
export struct SfItem {
    kind: SfItemKind;
    int_val: i64;
    bool_val: bool;
    str_val: []const u8;
}

/// RFC 8941 参数键值对（`;key=val` 或 `;flag`）
export struct SfParam {
    key: []const u8;
    val: SfItem;
}

type SfvVec16 = @Vector(16, u8);

const SFV_TCHAR: u8 = 1;
const SFV_TOKEN: u8 = 2;
const SFV_B64: u8 = 4;
const SFV_KEY: u8 = 8;

fn makeSfvClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        let mask: u8 = 0;
        const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const is_digit = c >= '0' && c <= '9';
        const is_tchar = is_alpha || is_digit || c == '!' || c == '#' || c == '$' || c == '%'
            || c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
            || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
        if (is_tchar) {
            mask |= SFV_TCHAR | SFV_TOKEN;
        }
        if (c == ':' || c == '/') {
            mask |= SFV_TOKEN;
        }
        if (is_alpha || is_digit || c == '+' || c == '/' || c == '=') {
            mask |= SFV_B64;
        }
        if ((c >= 'a' && c <= 'z') || is_digit || c == '_' || c == '-' || c == '.' || c == '*') {
            mask |= SFV_KEY;
        }
        lut[i] = mask;
    }
    return lut;
}

const SFV_CLASS_LUT: [u8; 256] = makeSfvClassLut();

fn sfvSimdSkipSpaces(s: []const u8, start: usize): usize {
    let pos = start;
    const v_sp: SfvVec16 = @splat(' ' as u8);
    while (pos + 16 <= s.len) {
        const chunk: SfvVec16 = s[pos .. pos + 16][0..16].*;
        const mask: u16 = @bitCast(chunk != v_sp);
        if (mask != 0) {
            return pos + usize(@ctz(mask));
        }
        pos += 16;
    }
    while (pos < s.len && s[pos] == ' ') {
        pos += 1;
    }
    return pos;
}

fn sfvSimdScanStringPlain(s: []const u8, start: usize): usize {
    let pos = start;
    const v_lo: SfvVec16 = @splat(0x20 as u8);
    const v_hi: SfvVec16 = @splat(0x7E as u8);
    const v_qt: SfvVec16 = @splat('"' as u8);
    const v_bs: SfvVec16 = @splat('\\' as u8);
    while (pos + 16 <= s.len) {
        const chunk: SfvVec16 = s[pos .. pos + 16][0..16].*;
        const m_lo: u16 = @bitCast(chunk < v_lo);
        const m_hi: u16 = @bitCast(chunk > v_hi);
        const m_qt: u16 = @bitCast(chunk == v_qt);
        const m_bs: u16 = @bitCast(chunk == v_bs);
        const stop_mask: u16 = m_lo | m_hi | m_qt | m_bs;
        if (stop_mask != 0) {
            return pos + usize(@ctz(stop_mask));
        }
        pos += 16;
    }
    return pos;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isTchar(c: u8): bool {
    return (SFV_CLASS_LUT[usize(c)] & SFV_TCHAR) != 0;
}

fn isTokenChar(c: u8): bool {
    return (SFV_CLASS_LUT[usize(c)] & SFV_TOKEN) != 0;
}

fn isBase64Char(c: u8): bool {
    return (SFV_CLASS_LUT[usize(c)] & SFV_B64) != 0;
}

fn isKeyStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || c == '*';
}

fn isKeyChar(c: u8): bool {
    return (SFV_CLASS_LUT[usize(c)] & SFV_KEY) != 0;
}

/// 解析单个 RFC 8941 标量项（Boolean / Integer / Decimal / String / Token / Byte Sequence）
/// 成功时将消耗字节数写入 consumed 并返回 SfItem；语法非法返回 null
export fn parseSfItem(src: []const u8, consumed: *usize): SfItem? {
    let pos: usize = sfvSimdSkipSpaces(src, 0);
    if (pos >= src.len) {
        return null;
    }

    const first = src[pos];

    // 1. Boolean: ?1 / ?0
    if (first == '?') {
        if (pos + 1 >= src.len) {
            return null;
        }
        const b = src[pos + 1];
        if (b != '0' && b != '1') {
            return null;
        }
        consumed.* = pos + 2;
        return SfItem{
            kind: SfItemKind.boolean,
            int_val: 0,
            bool_val: b == '1',
            str_val: src[pos..pos + 2],
        };
    }

    // 2. String: "..." (仅允许 0x20..0x7E，仅支持 \" 与 \\ 转义)
    if (first == '"') {
        pos += 1;
        const start = pos;
        while (pos < src.len) {
            pos = sfvSimdScanStringPlain(src, pos);
            if (pos >= src.len) {
                return null;
            }
            const c = src[pos];
            if (c == '\\') {
                if (pos + 1 >= src.len) {
                    return null;
                }
                const nxt = src[pos + 1];
                if (nxt != '"' && nxt != '\\') {
                    return null;
                }
                pos += 2;
                continue;
            }
            if (c == '"') {
                const slice = src[start..pos];
                consumed.* = pos + 1;
                return SfItem{
                    kind: SfItemKind.string,
                    int_val: 0,
                    bool_val: false,
                    str_val: slice,
                };
            }
            if (c < 0x20 || c > 0x7E) {
                return null;
            }
            pos += 1;
        }
        return null;
    }

    // 3. Byte Sequence: :base64:
    if (first == ':') {
        pos += 1;
        const start = pos;
        while (pos < src.len && src[pos] != ':') {
            if (!isBase64Char(src[pos])) {
                return null;
            }
            pos += 1;
        }
        if (pos >= src.len || src[pos] != ':') {
            return null;
        }
        const slice = src[start..pos];
        consumed.* = pos + 1;
        return SfItem{
            kind: SfItemKind.byte_seq,
            int_val: 0,
            bool_val: false,
            str_val: slice,
        };
    }

    // 4. Integer / Decimal
    if (first == '-' || isDigit(first)) {
        const start = pos;
        let negative = false;
        if (first == '-') {
            negative = true;
            pos += 1;
            if (pos >= src.len || !isDigit(src[pos])) {
                return null;
            }
        }

        let int_digits: usize = 0;
        let int_part: i64 = 0;
        while (pos < src.len && isDigit(src[pos])) {
            int_digits += 1;
            if (int_digits > 15) {
                return null;
            }
            int_part = int_part * 10 + i64(src[pos] - '0');
            pos += 1;
        }

        if (pos < src.len && src[pos] == '.') {
            // Decimal: 整数部分最多 12 位，小数部分 1..3 位
            if (int_digits > 12) {
                return null;
            }
            pos += 1;
            let frac_digits: usize = 0;
            let frac_part: i64 = 0;
            while (pos < src.len && isDigit(src[pos])) {
                frac_digits += 1;
                if (frac_digits > 3) {
                    return null;
                }
                frac_part = frac_part * 10 + i64(src[pos] - '0');
                pos += 1;
            }
            if (frac_digits == 0) {
                return null;
            }
            if (frac_digits == 1) {
                frac_part *= 100;
            } else if (frac_digits == 2) {
                frac_part *= 10;
            }
            let milli: i64 = int_part * 1000 + frac_part;
            if (negative) {
                milli = -milli;
            }
            consumed.* = pos;
            return SfItem{
                kind: SfItemKind.decimal_milli,
                int_val: milli,
                bool_val: false,
                str_val: src[start..pos],
            };
        }

        let signed_val: i64 = if (negative) (-int_part) else int_part;
        consumed.* = pos;
        return SfItem{
            kind: SfItemKind.integer,
            int_val: signed_val,
            bool_val: false,
            str_val: src[start..pos],
        };
    }

    // 5. Token: (ALPHA / '*') *(tchar / ':' / '/')
    if (isAlpha(first) || first == '*') {
        const start = pos;
        pos += 1;
        while (pos < src.len && isTokenChar(src[pos])) {
            pos += 1;
        }
        consumed.* = pos;
        return SfItem{
            kind: SfItemKind.token,
            int_val: 0,
            bool_val: false,
            str_val: src[start..pos],
        };
    }

    return null;
}

/// RFC 8941 参数列表迭代器（解析 `;k1=v1;flag;k2="str"`）
export struct SfParamIter {
    src: []const u8;
    pos: usize;
    had_error: bool;

    static fn init(params_src: []const u8): SfParamIter {
        return SfParamIter{
            src: params_src,
            pos: 0,
            had_error: false,
        };
    }

    fn hasError(): bool {
        return _.had_error;
    }

    /// 迭代下一个参数；无 `=` 时默认值为 `boolean(true)`
    fn next(*_): SfParam? {
        if (_.had_error) {
            return null;
        }
        while (_.pos < _.src.len && _.src[_.pos] == ' ') {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) {
            return null;
        }
        if (_.src[_.pos] != ';') {
            _.had_error = true;
            return null;
        }
        _.pos += 1;

        while (_.pos < _.src.len && _.src[_.pos] == ' ') {
            _.pos += 1;
        }
        if (_.pos >= _.src.len || !isKeyStart(_.src[_.pos])) {
            _.had_error = true;
            return null;
        }

        const key_start = _.pos;
        _.pos += 1;
        while (_.pos < _.src.len && isKeyChar(_.src[_.pos])) {
            _.pos += 1;
        }
        const key = _.src[key_start.._.pos];

        if (_.pos < _.src.len && _.src[_.pos] == '=') {
            _.pos += 1;
            let c: usize = 0;
            if (val := parseSfItem(_.src[_.pos..], &c)) {
                _.pos += c;
                return SfParam{
                    key: key,
                    val: val,
                };
            }
            _.had_error = true;
            return null;
        }

        return SfParam{
            key: key,
            val: SfItem{
                kind: SfItemKind.boolean,
                int_val: 0,
                bool_val: true,
                str_val: "",
            },
        };
    }
}

/// 序列化 RFC 8941 String（自动转义 `"` 与 `\`；拒绝不可打印 ASCII 字符）
export fn formatSfString(s: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;

    let i: usize = 0;
    while (i < s.len) {
        const plain_end = sfvSimdScanStringPlain(s, i);
        if (plain_end > i) {
            const run = plain_end - i;
            if (pos + run > out.len) return null;
            @memcpy(out[pos .. pos + run], s[i..plain_end]);
            pos += run;
            i = plain_end;
            if (i >= s.len) break;
        }
        const c = s[i];
        if (c < 0x20 || c > 0x7E) {
            return null;
        }
        if (c == '"' || c == '\\') {
            if (pos + 2 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }

    if (pos >= out.len) return null;
    out[pos] = '"';
    pos += 1;
    return out[0..pos];
}

/// 序列化 RFC 8941 Byte Sequence（`:base64:`，严格校验 Base64 字符集）
export fn formatSfByteSeq(b64: []const u8, out: []u8): ([]const u8)? {
    if (b64.len + 2 > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < b64.len) : (i += 1) {
        if (!isBase64Char(b64[i])) {
            return null;
        }
    }
    out[0] = ':';
    if (b64.len > 0) {
        @memcpy(out[1 .. 1 + b64.len], b64);
    }
    out[b64.len + 1] = ':';
    return out[0..b64.len + 2];
}
