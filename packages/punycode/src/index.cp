// packages/@cap/punycode/src/index.cp
// Zero-allocation RFC 3492 Punycode (IDNA xn--...) encoder and decoder.
// MIT clean-room pure CP implementation.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUTs, and @memcpy.

type PcVec16 = @Vector(16, u8);

pub const BASE: u32 = 36;
pub const TMIN: u32 = 1;
pub const TMAX: u32 = 26;
pub const SKEW: u32 = 38;
pub const DAMP: u32 = 700;
pub const INITIAL_BIAS: u32 = 72;
pub const INITIAL_N: u32 = 128;

const PC_ENC_DIGITS = "abcdefghijklmnopqrstuvwxyz0123456789";

fn pcBuildDecDigitLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [36; 256];
    let c: usize = 97;
    while (c <= 122) {
        lut[c] = (c - 97) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = (c - 65) as u8;
        c += 1;
    }
    c = 48;
    while (c <= 57) {
        lut[c] = (c - 48 + 26) as u8;
        c += 1;
    }
    return lut;
}

const PC_DEC_DIGIT_LUT: [256]u8 = pcBuildDecDigitLut();

fn pcToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn pcIsXnPrefix(s: []const u8): bool {
    if (s.len < 4) {
        return false;
    }
    return pcToLower(s[0]) == ('x' as u8) && pcToLower(s[1]) == ('n' as u8) && s[2] == ('-' as u8) && s[3] == ('-' as u8);
}

fn pcFindDot(s: string, start: usize): usize {
    let i = start;
    const len = s.len;
    const v_dot: PcVec16 = @splat('.' as u8);
    while (i + 16 <= len) {
        const v: PcVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_dot)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == ('.' as u8)) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != ('.' as u8)) {
        i += 1;
    }
    return i;
}

/// Returns true if all bytes in `s` are 7-bit ASCII (`< 0x80`).
pub fn isAsciiOnly(s: string): bool {
    let i: usize = 0;
    const len = s.len;
    const v_80: PcVec16 = @splat(0x80 as u8);
    while (i + 16 <= len) {
        const v: PcVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v >= v_80)) {
            return false;
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] >= (0x80 as u8)) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn pcAdapt(delta_in: u32, numpoints: u32, firsttime: bool): u32 {
    let delta: u32 = 0 as u32;
    if (firsttime) {
        delta = delta_in / DAMP;
    } else {
        delta = delta_in >> (1 as u5);
    }
    delta += delta / numpoints;
    let k: u32 = 0 as u32;
    const threshold: u32 = ((BASE - TMIN) * TMAX) / (2 as u32);
    while (delta > threshold) {
        delta = delta / (BASE - TMIN);
        k += BASE;
    }
    return k + (((BASE - TMIN + (1 as u32)) * delta) / (delta + SKEW));
}

fn pcEncodeDigit(d: u32): u8 {
    return PC_ENC_DIGITS[d as usize];
}

fn pcDecodeDigit(c: u8): u32 {
    return PC_DEC_DIGIT_LUT[c as usize] as u32;
}

fn pcThreshold(k: u32, bias: u32): u32 {
    if (k <= bias) {
        return TMIN;
    }
    if (k >= bias + TMAX) {
        return TMAX;
    }
    return k - bias;
}

/// Strictly decode UTF-8 bytes into Unicode code points in `cps` (max 256).
/// Returns number of code points or `-1` on malformed UTF-8 / capacity overflow.
fn pcDecodeUtf8(s: []const u8, cps: []u32): i64 {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < s.len) {
        if (count >= cps.len) {
            return -1 as i64;
        }
        const b0 = s[pos];
        if (b0 < (0x80 as u8)) {
            cps[count] = b0 as u32;
            count += 1;
            pos += 1;
        } else if ((b0 & (0xE0 as u8)) == (0xC0 as u8)) {
            if (pos + 1 >= s.len) { return -1 as i64; }
            const b1 = s[pos + 1];
            if ((b1 & (0xC0 as u8)) != (0x80 as u8)) { return -1 as i64; }
            const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((b1 & (0x3F as u8)) as u32);
            if (cp < (0x80 as u32)) { return -1 as i64; }
            cps[count] = cp;
            count += 1;
            pos += 2;
        } else if ((b0 & (0xF0 as u8)) == (0xE0 as u8)) {
            if (pos + 2 >= s.len) { return -1 as i64; }
            const b1 = s[pos + 1];
            const b2 = s[pos + 2];
            if ((b1 & (0xC0 as u8)) != (0x80 as u8) || (b2 & (0xC0 as u8)) != (0x80 as u8)) { return -1 as i64; }
            const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5)) | (((b1 & (0x3F as u8)) as u32) << (6 as u5)) | ((b2 & (0x3F as u8)) as u32);
            if (cp < (0x800 as u32) || (cp >= (0xD800 as u32) && cp <= (0xDFFF as u32))) { return -1 as i64; }
            cps[count] = cp;
            count += 1;
            pos += 3;
        } else if ((b0 & (0xF8 as u8)) == (0xF0 as u8)) {
            if (pos + 3 >= s.len) { return -1 as i64; }
            const b1 = s[pos + 1];
            const b2 = s[pos + 2];
            const b3 = s[pos + 3];
            if ((b1 & (0xC0 as u8)) != (0x80 as u8) || (b2 & (0xC0 as u8)) != (0x80 as u8) || (b3 & (0xC0 as u8)) != (0x80 as u8)) { return -1 as i64; }
            const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5)) | (((b1 & (0x3F as u8)) as u32) << (12 as u5)) | (((b2 & (0x3F as u8)) as u32) << (6 as u5)) | ((b3 & (0x3F as u8)) as u32);
            if (cp < (0x10000 as u32) || cp > (0x10FFFF as u32)) { return -1 as i64; }
            cps[count] = cp;
            count += 1;
            pos += 4;
        } else {
            return -1 as i64;
        }
    }
    return count as i64;
}

/// Encode Unicode code points `cps[0..len]` into UTF-8 bytes in `out`.
/// Returns bytes written or `-1` on insufficient `out` buffer.
fn pcEncodeUtf8(cps: []const u32, out: []u8): i64 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < cps.len) {
        const cp = cps[i];
        if (cp < (0x80 as u32)) {
            if (pos + 1 > out.len) { return -1 as i64; }
            out[pos] = cp as u8;
            pos += 1;
        } else if (cp < (0x800 as u32)) {
            if (pos + 2 > out.len) { return -1 as i64; }
            out[pos] = ((0xC0 as u32) | (cp >> (6 as u5))) as u8;
            out[pos + 1] = ((0x80 as u32) | (cp & (0x3F as u32))) as u8;
            pos += 2;
        } else if (cp < (0x10000 as u32)) {
            if (pos + 3 > out.len) { return -1 as i64; }
            out[pos] = ((0xE0 as u32) | (cp >> (12 as u5))) as u8;
            out[pos + 1] = ((0x80 as u32) | ((cp >> (6 as u5)) & (0x3F as u32))) as u8;
            out[pos + 2] = ((0x80 as u32) | (cp & (0x3F as u32))) as u8;
            pos += 3;
        } else {
            if (pos + 4 > out.len) { return -1 as i64; }
            out[pos] = ((0xF0 as u32) | (cp >> (18 as u5))) as u8;
            out[pos + 1] = ((0x80 as u32) | ((cp >> (12 as u5)) & (0x3F as u32))) as u8;
            out[pos + 2] = ((0x80 as u32) | ((cp >> (6 as u5)) & (0x3F as u32))) as u8;
            out[pos + 3] = ((0x80 as u32) | (cp & (0x3F as u32))) as u8;
            pos += 4;
        }
        i += 1;
    }
    return pos as i64;
}

/// Encode a single UTF-8 label into RFC 3492 ASCII Punycode (without `xn--` prefix).
/// Returns bytes written into `out`, or `-1` on invalid UTF-8 or insufficient buffer.
pub fn encodeLabel(utf8_label: string, out: []u8): i64 {
    let cps: [256]u32 = undefined;
    const cpLenI64 = pcDecodeUtf8(utf8_label, cps[0..256]);
    if (cpLenI64 < (0 as i64)) {
        return -1 as i64;
    }
    const cpLen = cpLenI64 as usize;

    let pos: usize = 0;
    let b: usize = 0;
    let idx: usize = 0;
    while (idx < cpLen) {
        if (cps[idx] < (0x80 as u32)) {
            if (pos >= out.len) {
                return -1 as i64;
            }
            out[pos] = cps[idx] as u8;
            pos += 1;
            b += 1;
        }
        idx += 1;
    }

    let h = b;
    if (b > 0) {
        if (pos >= out.len) {
            return -1 as i64;
        }
        out[pos] = '-' as u8;
        pos += 1;
    }

    let n: u32 = INITIAL_N;
    let delta: u64 = 0 as u64;
    let bias: u32 = INITIAL_BIAS;
    const maxDelta: u64 = 0x7FFFFFFF as u64;

    while (h < cpLen) {
        // Find minimum code point >= n.
        let m: u32 = 0x110000 as u32;
        let j: usize = 0;
        while (j < cpLen) {
            if (cps[j] >= n && cps[j] < m) {
                m = cps[j];
            }
            j += 1;
        }

        const step = ((m - n) as u64) * ((h + 1) as u64);
        if (step > maxDelta || delta + step > maxDelta) {
            return -1 as i64;
        }
        delta += step;
        n = m;

        let kidx: usize = 0;
        while (kidx < cpLen) {
            const c = cps[kidx];
            if (c < n) {
                delta += 1 as u64;
                if (delta > maxDelta) {
                    return -1 as i64;
                }
            } else if (c == n) {
                let q: u32 = delta as u32;
                let k: u32 = BASE;
                while (true) {
                    const t = pcThreshold(k, bias);
                    if (q < t) {
                        break;
                    }
                    if (pos >= out.len) {
                        return -1 as i64;
                    }
                    const rem = (q - t) % (BASE - t);
                    out[pos] = pcEncodeDigit(t + rem);
                    pos += 1;
                    q = (q - t) / (BASE - t);
                    k += BASE;
                }
                if (pos >= out.len) {
                    return -1 as i64;
                }
                out[pos] = pcEncodeDigit(q);
                pos += 1;
                bias = pcAdapt(delta as u32, (h + 1) as u32, h == b);
                delta = 0 as u64;
                h += 1;
            }
            kidx += 1;
        }

        delta += 1 as u64;
        n += 1 as u32;
    }

    return pos as i64;
}

/// Decode an RFC 3492 ASCII Punycode label (without `xn--` prefix) back into UTF-8.
/// Returns bytes written into `out_utf8`, or `-1` on invalid Punycode, overflow, or insufficient buffer.
pub fn decodeLabel(ascii_label: string, out_utf8: []u8): i64 {
    let cps: [256]u32 = undefined;
    let outLen: usize = 0;

    // Locate last '-' delimiter.
    let hasDash = false;
    let lastDash: usize = 0;
    let scan: usize = 0;
    while (scan < ascii_label.len) {
        if (ascii_label[scan] >= (0x80 as u8)) {
            return -1 as i64;
        }
        if (ascii_label[scan] == ('-' as u8)) {
            hasDash = true;
            lastDash = scan;
        }
        scan += 1;
    }

    let inIdx: usize = 0;
    if (hasDash) {
        if (lastDash > 256) {
            return -1 as i64;
        }
        let bi: usize = 0;
        while (bi < lastDash) {
            cps[bi] = ascii_label[bi] as u32;
            bi += 1;
        }
        outLen = lastDash;
        inIdx = lastDash + 1;
    }

    let n: u32 = INITIAL_N;
    let i: u64 = 0 as u64;
    let bias: u32 = INITIAL_BIAS;
    const maxInt: u64 = 0x7FFFFFFF as u64;

    while (inIdx < ascii_label.len) {
        const oldi = i;
        let w: u64 = 1 as u64;
        let k: u32 = BASE;

        while (true) {
            if (inIdx >= ascii_label.len) {
                return -1 as i64;
            }
            const digit = pcDecodeDigit(ascii_label[inIdx]);
            inIdx += 1;
            if (digit >= BASE) {
                return -1 as i64;
            }
            if ((digit as u64) > (maxInt - i) / w) {
                return -1 as i64;
            }
            i += (digit as u64) * w;

            const t = pcThreshold(k, bias);
            if (digit < t) {
                break;
            }
            const baseMinusT = (BASE - t) as u64;
            if (w > maxInt / baseMinusT) {
                return -1 as i64;
            }
            w = w * baseMinusT;
            k += BASE;
        }

        const outPlus1 = (outLen + 1) as u32;
        bias = pcAdapt((i - oldi) as u32, outPlus1, oldi == (0 as u64));

        const addN = i / (outPlus1 as u64);
        if (addN > ((0x10FFFF as u64) - (n as u64))) {
            return -1 as i64;
        }
        n += addN as u32;
        if (n >= (0xD800 as u32) && n <= (0xDFFF as u32)) {
            return -1 as i64;
        }
        i = i % (outPlus1 as u64);

        if (outLen >= 256) {
            return -1 as i64;
        }
        // Insert n at index i by shifting cps[i..outLen] right by 1.
        const insPos = i as usize;
        let shift = outLen;
        while (shift > insPos) {
            cps[shift] = cps[shift - 1];
            shift -= 1;
        }
        cps[insPos] = n;
        outLen += 1;
        i += 1 as u64;
    }

    return pcEncodeUtf8(cps[0..outLen], out_utf8);
}

fn pcWriteSlice(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    if (src.len > 0) {
        @memcpy(out[pos.* .. pos.* + src.len], src);
        pos.* += src.len;
    }
    return true;
}

/// Convert a UTF-8 internationalized domain name into ASCII (IDNA `xn--` per non-ASCII label).
/// Pure ASCII labels pass through verbatim. Returns bytes written, or `-1` on error/overflow.
pub fn toAsciiDomain(utf8_domain: string, out: []u8): i64 {
    if (utf8_domain.len == 0) {
        return 0 as i64;
    }
    let outPos: usize = 0;
    let pos: usize = 0;

    while (pos <= utf8_domain.len) {
        const dot = pcFindDot(utf8_domain, pos);
        const label = utf8_domain[pos..dot];

        if (isAsciiOnly(label)) {
            if (!pcWriteSlice(out, &outPos, label)) {
                return -1 as i64;
            }
        } else {
            if (!pcWriteSlice(out, &outPos, "xn--")) {
                return -1 as i64;
            }
            const enc = encodeLabel(label, out[outPos..out.len]);
            if (enc < (0 as i64)) {
                return -1 as i64;
            }
            outPos += enc as usize;
        }

        if (dot < utf8_domain.len) {
            if (!pcWriteSlice(out, &outPos, ".")) {
                return -1 as i64;
            }
            pos = dot + 1;
        } else {
            break;
        }
    }

    return outPos as i64;
}

/// Convert an ASCII IDNA domain name (`xn--...` labels) back into UTF-8 Unicode.
/// Non-`xn--` labels pass through verbatim. Returns bytes written, or `-1` on error/overflow.
pub fn toUnicodeDomain(ascii_domain: string, out: []u8): i64 {
    if (ascii_domain.len == 0) {
        return 0 as i64;
    }
    let outPos: usize = 0;
    let pos: usize = 0;

    while (pos <= ascii_domain.len) {
        const dot = pcFindDot(ascii_domain, pos);
        const label = ascii_domain[pos..dot];

        if (pcIsXnPrefix(label)) {
            const decLen = decodeLabel(label[4..label.len], out[outPos..out.len]);
            if (decLen < (0 as i64)) {
                return -1 as i64;
            }
            outPos += decLen as usize;
        } else {
            if (!pcWriteSlice(out, &outPos, label)) {
                return -1 as i64;
            }
        }

        if (dot < ascii_domain.len) {
            if (!pcWriteSlice(out, &outPos, ".")) {
                return -1 as i64;
            }
            pos = dot + 1;
        } else {
            break;
        }
    }

    return outPos as i64;
}

export {
    BASE,
    TMIN,
    TMAX,
    SKEW,
    DAMP,
    INITIAL_BIAS,
    INITIAL_N,
    encodeLabel,
    decodeLabel,
    toAsciiDomain,
    toUnicodeDomain,
    isAsciiOnly,
};
