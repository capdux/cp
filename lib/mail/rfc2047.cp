// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-22T11:12:13Z (RFC 2047 MIME Encoded-Word Codec)

type Rfc2047Vec16 = @Vector(16, u8);

const B64_TABLE: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn makeRfc2047HexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(u8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(u8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

fn makeRfc2047B64Lut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i - 'A'));
        } else if (i >= 'a' && i <= 'z') {
            lut[i] = @as(u8, @intCast(i - 'a' + 26));
        } else if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0' + 52));
        } else if (i == '+') {
            lut[i] = 62;
        } else if (i == '/') {
            lut[i] = 63;
        }
    }
    return lut;
}

fn makeRfc2047WsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[usize(' ')] = 1;
    lut[usize('\t')] = 1;
    lut[usize('\r')] = 1;
    lut[usize('\n')] = 1;
    return lut;
}

const RFC2047_HEX_LUT: [u8; 256] = makeRfc2047HexLut();
const RFC2047_B64_LUT: [u8; 256] = makeRfc2047B64Lut();
const RFC2047_WS_LUT: [u8; 256] = makeRfc2047WsLut();

fn findByteSimd(s: []const u8, start: usize, end_limit: usize, needle: u8): usize {
    let i = start;
    const v_needle: Rfc2047Vec16 = @splat(needle);
    while (i + 16 <= end_limit) : (i += 16) {
        const chunk: Rfc2047Vec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < end_limit) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return end_limit;
}

fn findQSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_us: Rfc2047Vec16 = @splat('_' as u8);
    const v_eq: Rfc2047Vec16 = @splat('=' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: Rfc2047Vec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((chunk == v_us) | (chunk == v_eq));
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '_' || s[i] == '=') return i;
    }
    return s.len;
}

fn findWsSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_sp: Rfc2047Vec16 = @splat(' ' as u8);
    const v_tb: Rfc2047Vec16 = @splat('\t' as u8);
    const v_cr: Rfc2047Vec16 = @splat('\r' as u8);
    const v_lf: Rfc2047Vec16 = @splat('\n' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: Rfc2047Vec16 = s[i .. i + 16][0..16].*;
        const hit = (chunk == v_sp) | (chunk == v_tb) | (chunk == v_cr) | (chunk == v_lf);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (RFC2047_WS_LUT[usize(s[i])] != 0) return i;
    }
    return s.len;
}

/// RFC 2047 编码方式（B=Base64, Q=Quoted-Printable）
export enum EncodedWordEncoding {
    base64,
    quoted_printable,
}

/// 解析后的 RFC 2047 Encoded-Word 结构（零拷贝切片）
export struct EncodedWord {
    charset: []const u8;
    encoding: EncodedWordEncoding;
    encoded_text: []const u8;
}

fn isSpace(c: u8): bool {
    return RFC2047_WS_LUT[usize(c)] != 0;
}

fn hexNibble(c: u8): u8? {
    const v = RFC2047_HEX_LUT[usize(c)];
    if (v == 0xFF) return null;
    return v;
}

fn b64Val(c: u8): u8? {
    const v = RFC2047_B64_LUT[usize(c)];
    if (v == 0xFF) return null;
    return v;
}

/// 严格解析单个 RFC 2047 Encoded-Word（`=?charset?B|b|Q|q?text?=`）
export fn parseEncodedWord(token: []const u8): EncodedWord? {
    if (token.len < 8) {
        return null;
    }
    if (token[0] != '=' || token[1] != '?' || token[token.len - 2] != '?' || token[token.len - 1] != '=') {
        return null;
    }
    // RFC 2047 §2：encoded-word 内不得包含空白或控制字符（16-lane SIMD 校验）
    let vi: usize = 0;
    const v_lo: Rfc2047Vec16 = @splat(0x20 as u8);
    const v_hi: Rfc2047Vec16 = @splat(0x7E as u8);
    while (vi + 16 <= token.len) : (vi += 16) {
        const chunk: Rfc2047Vec16 = token[vi .. vi + 16][0..16].*;
        const bad = (chunk <= v_lo) | (chunk > v_hi);
        const mask: u16 = @bitCast(bad);
        if (mask != 0) return null;
    }
    while (vi < token.len) : (vi += 1) {
        const c = token[vi];
        if (c <= 0x20 || c > 0x7E) {
            return null;
        }
    }

    // 查找 charset 后的第一个 '?'
    const q1_idx = findByteSimd(token, 2, token.len - 2, '?');
    if (q1_idx >= token.len - 2) {
        return null;
    }
    const charset = token[2..q1_idx];
    if (charset.len == 0) {
        return null;
    }

    // encoding 必须恰好 1 字节且后紧跟 '?'
    if (q1_idx + 2 >= token.len - 1 || token[q1_idx + 2] != '?') {
        return null;
    }
    const enc_ch = token[q1_idx + 1];
    let encoding = EncodedWordEncoding.base64;
    if (enc_ch == 'B' || enc_ch == 'b') {
        encoding = EncodedWordEncoding.base64;
    } else if (enc_ch == 'Q' || enc_ch == 'q') {
        encoding = EncodedWordEncoding.quoted_printable;
    } else {
        return null;
    }

    const text = token[q1_idx + 3..token.len - 2];
    if (findByteSimd(text, 0, text.len, '?') < text.len) {
        return null;
    }

    return EncodedWord{
        charset: charset,
        encoding: encoding,
        encoded_text: text,
    };
}

fn decodeQText(src: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let pos: usize = 0;
    while (i < src.len) {
        const next = findQSpecialSimd(src, i);
        if (next > i) {
            const seg_len = next - i;
            if (pos + seg_len > out.len) return null;
            @memcpy(out[pos .. pos + seg_len], src[i..next]);
            pos += seg_len;
        }
        if (next >= src.len) break;
        i = next;
        if (pos >= out.len) return null;
        const c = src[i];
        if (c == '_') {
            out[pos] = ' ';
            pos += 1;
            i += 1;
        } else {
            if (i + 2 >= src.len) return null;
            const hi = hexNibble(src[i + 1]) ?? return null;
            const lo = hexNibble(src[i + 2]) ?? return null;
            out[pos] = (hi << 4) | lo;
            pos += 1;
            i += 3;
        }
    }
    return out[0..pos];
}

fn decodeB64Text(src: []const u8, out: []u8): ([]const u8)? {
    if (src.len == 0) {
        return out[0..0];
    }
    if ((src.len % 4) != 0) {
        return null;
    }
    let i: usize = 0;
    let pos: usize = 0;
    while (i < src.len) {
        const c0 = src[i];
        const c1 = src[i + 1];
        const c2 = src[i + 2];
        const c3 = src[i + 3];

        const v0 = b64Val(c0) ?? return null;
        const v1 = b64Val(c1) ?? return null;

        if (pos >= out.len) return null;
        out[pos] = (v0 << 2) | (v1 >> 4);
        pos += 1;

        if (c2 == '=') {
            if (c3 != '=' || i + 4 != src.len) return null;
            break;
        }
        const v2 = b64Val(c2) ?? return null;
        if (pos >= out.len) return null;
        out[pos] = ((v1 & 0x0F) << 4) | (v2 >> 2);
        pos += 1;

        if (c3 == '=') {
            if (i + 4 != src.len) return null;
            break;
        }
        const v3 = b64Val(c3) ?? return null;
        if (pos >= out.len) return null;
        out[pos] = ((v2 & 0x03) << 6) | v3;
        pos += 1;

        i += 4;
    }
    return out[0..pos];
}

/// 解码单个 RFC 2047 Encoded-Word 写入 out（支持 B 与 Q 编码，零堆分配）
export fn decodeEncodedWord(token: []const u8, out: []u8): ([]const u8)? {
    const ew = parseEncodedWord(token) ?? return null;
    return switch (ew.encoding) {
        .base64 => decodeB64Text(ew.encoded_text, out),
        .quoted_printable => decodeQText(ew.encoded_text, out),
    };
}

fn copySlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
    return true;
}

/// 解码混合普通文本与多个 RFC 2047 Encoded-Word 的头部值
/// 按 RFC 2047 §6.2 自动折叠相邻 encoded-word 之间的纯空白；非法词原样保留
export fn decodeHeaderValue(header: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let pos: usize = 0;
    let prev_was_ew = false;
    let pending_ws: []const u8 = "";

    while (i < header.len) {
        if (isSpace(header[i])) {
            const ws_start = i;
            while (i < header.len && isSpace(header[i])) {
                i += 1;
            }
            pending_ws = header[ws_start..i];
            continue;
        }

        const w_start = i;
        i = findWsSimd(header, i);
        const word = header[w_start..i];

        if (parseEncodedWord(word) != null) {
            const saved_pos = pos;
            if (!prev_was_ew && pending_ws.len > 0) {
                if (!copySlice(out, &pos, pending_ws)) return null;
            }
            if (decoded := decodeEncodedWord(word, out[pos..])) {
                pos += decoded.len;
                prev_was_ew = true;
                pending_ws = "";
                continue;
            }
            // 若内部 Base64/Q 数据非法，按 RFC 2047 宽容回退为原样保留
            pos = saved_pos;
        }

        if (pending_ws.len > 0) {
            if (!copySlice(out, &pos, pending_ws)) return null;
            pending_ws = "";
        }
        if (!copySlice(out, &pos, word)) return null;
        prev_was_ew = false;
    }

    if (pending_ws.len > 0) {
        if (!copySlice(out, &pos, pending_ws)) return null;
    }
    return out[0..pos];
}

/// 将 UTF-8 字节串编码为 RFC 2047 `=?UTF-8?B?<base64>?=`（零堆分配）
export fn encodeUtf8Base64Word(text: []const u8, out: []u8): ([]const u8)? {
    const b64_len: usize = ((text.len + 2) / 3) * 4;
    const prefix = "=?UTF-8?B?";
    const suffix = "?=";
    const needed = prefix.len + b64_len + suffix.len;
    if (out.len < needed) {
        return null;
    }

    @memcpy(out[0..prefix.len], prefix);
    let pos: usize = prefix.len;

    let i: usize = 0;
    while (i + 3 <= text.len) {
        const b0 = text[i];
        const b1 = text[i + 1];
        const b2 = text[i + 2];
        out[pos] = B64_TABLE[usize(b0 >> 2)];
        out[pos + 1] = B64_TABLE[usize(((b0 & 0x03) << 4) | (b1 >> 4))];
        out[pos + 2] = B64_TABLE[usize(((b1 & 0x0F) << 2) | (b2 >> 6))];
        out[pos + 3] = B64_TABLE[usize(b2 & 0x3F)];
        pos += 4;
        i += 3;
    }

    const rem = text.len - i;
    if (rem == 1) {
        const b0 = text[i];
        out[pos] = B64_TABLE[usize(b0 >> 2)];
        out[pos + 1] = B64_TABLE[usize((b0 & 0x03) << 4)];
        out[pos + 2] = '=';
        out[pos + 3] = '=';
        pos += 4;
    } else if (rem == 2) {
        const b0 = text[i];
        const b1 = text[i + 1];
        out[pos] = B64_TABLE[usize(b0 >> 2)];
        out[pos + 1] = B64_TABLE[usize(((b0 & 0x03) << 4) | (b1 >> 4))];
        out[pos + 2] = B64_TABLE[usize((b1 & 0x0F) << 2)];
        out[pos + 3] = '=';
        pos += 4;
    }

    out[pos] = '?';
    out[pos + 1] = '=';
    pos += 2;
    return out[0..pos];
}
