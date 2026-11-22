// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T14:55:36Z (RFC 3492 Punycode Codec)

const BASE: u32 = 36;
const TMIN: u32 = 1;
const TMAX: u32 = 26;
const SKEW: u32 = 38;
const DAMP: u32 = 700;
const INITIAL_BIAS: u32 = 72;
const INITIAL_N: u32 = 128;
const U32_MAX: u32 = 0xFFFF_FFFF;
const MAX_CODEPOINTS: usize = 256;

/// 判断字节切片是否全部由 ASCII 字符（0x00..0x7F）组成
export fn isAsciiOnly(s: []const u8): bool {
    for (c of s) {
        if (c >= 0x80) {
            return false;
        }
    }
    return true;
}

fn adaptBias(delta_in: u32, num_points: u32, first_time: bool): u32 {
    let delta: u32 = if (first_time) (delta_in / DAMP) else (delta_in >> 1);
    delta += delta / num_points;
    let k: u32 = 0;
    const threshold: u32 = ((BASE - TMIN) * TMAX) / 2;
    while (delta > threshold) {
        delta /= (BASE - TMIN);
        k += BASE;
    }
    return k + (((BASE - TMIN + 1) * delta) / (delta + SKEW));
}

fn encodeDigit(d: u32): u8 {
    if (d < 26) {
        return u8('a') + u8(d);
    }
    return u8('0') + u8(d - 26);
}

fn decodeDigit(c: u8): u32? {
    if (c >= 'a' && c <= 'z') return u32(c - 'a');
    if (c >= 'A' && c <= 'Z') return u32(c - 'A');
    if (c >= '0' && c <= '9') return u32(c - '0') + 26;
    return null;
}

fn isValidScalar(cp: u32): bool {
    if (cp > 0x10_FFFF) return false;
    if (cp >= 0xD800 && cp <= 0xDFFF) return false;
    return true;
}

fn decodeUtf8ToCodepoints(utf8: []const u8, out_cps: []u32): usize? {
    let i: usize = 0;
    let count: usize = 0;

    while (i < utf8.len) {
        if (count >= out_cps.len) {
            return null;
        }
        const b0 = utf8[i];
        if (b0 < 0x80) {
            out_cps[count] = u32(b0);
            count += 1;
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0) {
            if (i + 1 >= utf8.len) return null;
            const b1 = utf8[i + 1];
            if ((b1 & 0xC0) != 0x80) return null;
            const cp: u32 = (u32(b0 & 0x1F) << 6) | u32(b1 & 0x3F);
            if (cp < 0x80) return null;
            out_cps[count] = cp;
            count += 1;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0) {
            if (i + 2 >= utf8.len) return null;
            const b1 = utf8[i + 1];
            const b2 = utf8[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return null;
            const cp: u32 = (u32(b0 & 0x0F) << 12) | (u32(b1 & 0x3F) << 6) | u32(b2 & 0x3F);
            if (cp < 0x800 || !isValidScalar(cp)) return null;
            out_cps[count] = cp;
            count += 1;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0) {
            if (i + 3 >= utf8.len) return null;
            const b1 = utf8[i + 1];
            const b2 = utf8[i + 2];
            const b3 = utf8[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return null;
            const cp: u32 = (u32(b0 & 0x07) << 18)
                | (u32(b1 & 0x3F) << 12)
                | (u32(b2 & 0x3F) << 6)
                | u32(b3 & 0x3F);
            if (cp < 0x1_0000 || !isValidScalar(cp)) return null;
            out_cps[count] = cp;
            count += 1;
            i += 4;
        } else {
            return null;
        }
    }

    return count;
}

fn encodeCodepointsToUtf8(cps: []const u32, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    for (cp of cps) {
        if (!isValidScalar(cp)) {
            return null;
        }
        if (cp < 0x80) {
            if (pos >= out.len) return null;
            out[pos] = u8(cp);
            pos += 1;
        } else if (cp < 0x800) {
            if (pos + 2 > out.len) return null;
            out[pos] = u8(0xC0) | u8(cp >> 6);
            out[pos + 1] = u8(0x80) | u8(cp & 0x3F);
            pos += 2;
        } else if (cp < 0x1_0000) {
            if (pos + 3 > out.len) return null;
            out[pos] = u8(0xE0) | u8(cp >> 12);
            out[pos + 1] = u8(0x80) | u8((cp >> 6) & 0x3F);
            out[pos + 2] = u8(0x80) | u8(cp & 0x3F);
            pos += 3;
        } else {
            if (pos + 4 > out.len) return null;
            out[pos] = u8(0xF0) | u8(cp >> 18);
            out[pos + 1] = u8(0x80) | u8((cp >> 12) & 0x3F);
            out[pos + 2] = u8(0x80) | u8((cp >> 6) & 0x3F);
            out[pos + 3] = u8(0x80) | u8(cp & 0x3F);
            pos += 4;
        }
    }
    return out[0..pos];
}

/// RFC 3492 §6.3 Punycode 编码器（UTF-8 -> 纯 ASCII Punycode，零堆分配）
export fn encodePunycode(utf8_label: []const u8, out: []u8): ([]const u8)? {
    let cps: [MAX_CODEPOINTS]u32 = undefined;
    const cp_len = decodeUtf8ToCodepoints(utf8_label, &cps) ?? return null;

    let out_pos: usize = 0;
    let basic_count: usize = 0;
    let i: usize = 0;
    while (i < cp_len) {
        if (cps[i] < 0x80) {
            if (out_pos >= out.len) return null;
            out[out_pos] = u8(cps[i]);
            out_pos += 1;
            basic_count += 1;
        }
        i += 1;
    }

    let handled: usize = basic_count;
    if (basic_count > 0) {
        if (out_pos >= out.len) return null;
        out[out_pos] = '-';
        out_pos += 1;
    }

    let n: u32 = INITIAL_N;
    let delta: u32 = 0;
    let bias: u32 = INITIAL_BIAS;

    while (handled < cp_len) {
        let m: u32 = U32_MAX;
        let idx: usize = 0;
        while (idx < cp_len) {
            const c = cps[idx];
            if (c >= n && c < m) {
                m = c;
            }
            idx += 1;
        }

        const handled_p1: u32 = u32(handled + 1);
        if (m - n > (U32_MAX - delta) / handled_p1) {
            return null;
        }
        delta += (m - n) * handled_p1;
        n = m;

        idx = 0;
        while (idx < cp_len) {
            const c = cps[idx];
            if (c < n) {
                if (delta == U32_MAX) return null;
                delta += 1;
            } else if (c == n) {
                let q: u32 = delta;
                let k: u32 = BASE;
                while (true) {
                    const t: u32 = if (k <= bias) TMIN else if (k >= bias + TMAX) TMAX else (k - bias);
                    if (q < t) {
                        break;
                    }
                    const digit = t + ((q - t) % (BASE - t));
                    if (out_pos >= out.len) return null;
                    out[out_pos] = encodeDigit(digit);
                    out_pos += 1;
                    q = (q - t) / (BASE - t);
                    k += BASE;
                }
                if (out_pos >= out.len) return null;
                out[out_pos] = encodeDigit(q);
                out_pos += 1;

                bias = adaptBias(delta, u32(handled + 1), handled == basic_count);
                delta = 0;
                handled += 1;
            }
            idx += 1;
        }

        if (delta == U32_MAX || n == U32_MAX) return null;
        delta += 1;
        n += 1;
    }

    return out[0..out_pos];
}

/// RFC 3492 §6.2 Punycode 解码器（ASCII Punycode -> UTF-8，零堆分配，防溢出与非法码点拦截）
export fn decodePunycode(ascii_input: []const u8, out: []u8): ([]const u8)? {
    if (!isAsciiOnly(ascii_input)) {
        return null;
    }

    let cps: [MAX_CODEPOINTS]u32 = undefined;
    let cp_len: usize = 0;

    // 查找最后一个 '-' 分隔符
    let delim_pos: usize? = null;
    let scan: usize = 0;
    while (scan < ascii_input.len) {
        if (ascii_input[scan] == '-') {
            delim_pos = scan;
        }
        scan += 1;
    }

    let in_idx: usize = 0;
    if (dp := delim_pos) {
        if (dp > MAX_CODEPOINTS) {
            return null;
        }
        let j: usize = 0;
        while (j < dp) {
            cps[j] = u32(ascii_input[j]);
            j += 1;
        }
        cp_len = dp;
        in_idx = dp + 1;
    }

    let n: u32 = INITIAL_N;
    let i: u32 = 0;
    let bias: u32 = INITIAL_BIAS;

    while (in_idx < ascii_input.len) {
        const oldi: u32 = i;
        let w: u32 = 1;
        let k: u32 = BASE;

        while (true) {
            if (in_idx >= ascii_input.len) {
                return null;
            }
            const digit = decodeDigit(ascii_input[in_idx]) ?? return null;
            in_idx += 1;

            if (digit > (U32_MAX - i) / w) {
                return null;
            }
            i += digit * w;

            const t: u32 = if (k <= bias) TMIN else if (k >= bias + TMAX) TMAX else (k - bias);
            if (digit < t) {
                break;
            }
            if (w > U32_MAX / (BASE - t)) {
                return null;
            }
            w *= (BASE - t);
            k += BASE;
        }

        const out_p1: u32 = u32(cp_len + 1);
        bias = adaptBias(i - oldi, out_p1, oldi == 0);

        const n_inc: u32 = i / out_p1;
        if (n_inc > 0x10_FFFF - n) {
            return null;
        }
        n += n_inc;
        i %= out_p1;

        if (!isValidScalar(n) || cp_len >= MAX_CODEPOINTS) {
            return null;
        }

        // 在下标 i 处插入新码点 n
        const insert_at: usize = usize(i);
        let shift_pos: usize = cp_len;
        while (shift_pos > insert_at) {
            cps[shift_pos] = cps[shift_pos - 1];
            shift_pos -= 1;
        }
        cps[insert_at] = n;
        cp_len += 1;
        i += 1;
    }

    return encodeCodepointsToUtf8(cps[0..cp_len], out);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn hasXnPrefix(s: []const u8): bool {
    if (s.len < 4) return false;
    return toLowerAscii(s[0]) == 'x'
        && toLowerAscii(s[1]) == 'n'
        && s[2] == '-'
        && s[3] == '-';
}

/// 编码单个 IDN 域名标签：纯 ASCII 原样拷贝，含非 ASCII 时输出 `xn--<punycode>`
export fn encodeIdnLabel(label: []const u8, out: []u8): ([]const u8)? {
    if (isAsciiOnly(label)) {
        if (out.len < label.len) return null;
        let i: usize = 0;
        while (i < label.len) {
            out[i] = label[i];
            i += 1;
        }
        return out[0..label.len];
    }

    if (out.len < 4) return null;
    out[0] = 'x';
    out[1] = 'n';
    out[2] = '-';
    out[3] = '-';
    const encoded = encodePunycode(label, out[4..]) ?? return null;
    return out[0..4 + encoded.len];
}

/// 解码单个 IDN 域名标签：大小写不敏感识别 `xn--` 前缀并解码回 UTF-8，否则原样拷贝
export fn decodeIdnLabel(label: []const u8, out: []u8): ([]const u8)? {
    if (hasXnPrefix(label)) {
        return decodePunycode(label[4..], out);
    }
    if (out.len < label.len) return null;
    let i: usize = 0;
    while (i < label.len) {
        out[i] = label[i];
        i += 1;
    }
    return out[0..label.len];
}
