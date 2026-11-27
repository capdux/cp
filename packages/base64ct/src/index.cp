// @cap/base64ct — Pure CP zero-allocation constant-time Base64 codec
// Parity with RustCrypto `base64ct` (table-free branchless bitwise arithmetic, strict RFC 4648 canonical decoding).

pub struct DecodeResult {
    pub len: usize,
    pub ok: bool,
}

fn b64ctInRange(c: u32, lo: u32, hi: u32): u32 {
    let ge_lo: u32 = ((c + 512) - lo) >> 9;
    let le_hi: u32 = ((hi + 512) - c) >> 9;
    return ge_lo & le_hi;
}

fn b64ctEq(c: u32, target: u32): u32 {
    return b64ctInRange(c, target, target);
}

// Constant-time branchless 6-bit -> ASCII mapping (zero lookup tables)
fn b64ctEncode6Std(x: u8): u8 {
    let v: u32 = (x & 0x3F) as u32;
    let ge26: u32 = ((v + 512) - 26) >> 9;
    let ge52: u32 = ((v + 512) - 52) >> 9;
    let ge62: u32 = ((v + 512) - 62) >> 9;
    let ge63: u32 = ((v + 512) - 63) >> 9;
    let ch: i32 = (v as i32) + 65
        + (ge26 as i32) * 6
        - (ge52 as i32) * 75
        - (ge62 as i32) * 15
        + (ge63 as i32) * 3;
    return ch as u8;
}

fn b64ctEncode6Url(x: u8): u8 {
    let v: u32 = (x & 0x3F) as u32;
    let ge26: u32 = ((v + 512) - 26) >> 9;
    let ge52: u32 = ((v + 512) - 52) >> 9;
    let ge62: u32 = ((v + 512) - 62) >> 9;
    let ge63: u32 = ((v + 512) - 63) >> 9;
    let ch: i32 = (v as i32) + 65
        + (ge26 as i32) * 6
        - (ge52 as i32) * 75
        - (ge62 as i32) * 13
        + (ge63 as i32) * 49;
    return ch as u8;
}

// Constant-time branchless ASCII -> 6-bit mapping (returns bit 0x80 set on invalid byte)
fn b64ctDecode6Std(ch: u8): u32 {
    let c: u32 = ch as u32;
    let m_upper = b64ctInRange(c, 65, 90);
    let m_lower = b64ctInRange(c, 97, 122);
    let m_digit = b64ctInRange(c, 48, 57);
    let m_62 = b64ctEq(c, 43);
    let m_63 = b64ctEq(c, 47);
    let valid = m_upper | m_lower | m_digit | m_62 | m_63;
    let val_i: i32 = (m_upper as i32) * ((c as i32) - 65)
        + (m_lower as i32) * ((c as i32) - 71)
        + (m_digit as i32) * ((c as i32) + 4)
        + (m_62 as i32) * 62
        + (m_63 as i32) * 63;
    return (val_i as u32) | ((valid ^ 1) << 7);
}

fn b64ctDecode6Url(ch: u8): u32 {
    let c: u32 = ch as u32;
    let m_upper = b64ctInRange(c, 65, 90);
    let m_lower = b64ctInRange(c, 97, 122);
    let m_digit = b64ctInRange(c, 48, 57);
    let m_62 = b64ctEq(c, 45);
    let m_63 = b64ctEq(c, 95);
    let valid = m_upper | m_lower | m_digit | m_62 | m_63;
    let val_i: i32 = (m_upper as i32) * ((c as i32) - 65)
        + (m_lower as i32) * ((c as i32) - 71)
        + (m_digit as i32) * ((c as i32) + 4)
        + (m_62 as i32) * 62
        + (m_63 as i32) * 63;
    return (val_i as u32) | ((valid ^ 1) << 7);
}

pub fn encodedLen(n: usize, padded: bool): usize {
    if (n == 0) {
        return 0;
    }
    if (padded) {
        return ((n + 2) / 3) * 4;
    }
    let rem = n % 3;
    let base = (n / 3) * 4;
    if (rem == 0) {
        return base;
    }
    return base + rem + 1;
}

fn b64ctEncodeCore(src: []u8, out: []u8, url_safe: bool, padded: bool): usize {
    let need = encodedLen(src.len, padded);
    if (need == 0) {
        return 0;
    }
    if (out.len < need) {
        return 0;
    }

    let i: usize = 0;
    let o: usize = 0;
    while (i + 3 <= src.len) {
        let b0 = src[i];
        let b1 = src[i + 1];
        let b2 = src[i + 2];
        let s0: u8 = b0 >> 2;
        let s1: u8 = ((b0 & 0x03) << 4) | (b1 >> 4);
        let s2: u8 = ((b1 & 0x0F) << 2) | (b2 >> 6);
        let s3: u8 = b2 & 0x3F;
        if (url_safe) {
            out[o] = b64ctEncode6Url(s0);
            out[o + 1] = b64ctEncode6Url(s1);
            out[o + 2] = b64ctEncode6Url(s2);
            out[o + 3] = b64ctEncode6Url(s3);
        } else {
            out[o] = b64ctEncode6Std(s0);
            out[o + 1] = b64ctEncode6Std(s1);
            out[o + 2] = b64ctEncode6Std(s2);
            out[o + 3] = b64ctEncode6Std(s3);
        }
        i = i + 3;
        o = o + 4;
    }

    let rem = src.len - i;
    if (rem == 1) {
        let b0 = src[i];
        let s0: u8 = b0 >> 2;
        let s1: u8 = (b0 & 0x03) << 4;
        if (url_safe) {
            out[o] = b64ctEncode6Url(s0);
            out[o + 1] = b64ctEncode6Url(s1);
        } else {
            out[o] = b64ctEncode6Std(s0);
            out[o + 1] = b64ctEncode6Std(s1);
        }
        o = o + 2;
        if (padded) {
            out[o] = '=' as u8;
            out[o + 1] = '=' as u8;
            o = o + 2;
        }
    } else if (rem == 2) {
        let b0 = src[i];
        let b1 = src[i + 1];
        let s0: u8 = b0 >> 2;
        let s1: u8 = ((b0 & 0x03) << 4) | (b1 >> 4);
        let s2: u8 = (b1 & 0x0F) << 2;
        if (url_safe) {
            out[o] = b64ctEncode6Url(s0);
            out[o + 1] = b64ctEncode6Url(s1);
            out[o + 2] = b64ctEncode6Url(s2);
        } else {
            out[o] = b64ctEncode6Std(s0);
            out[o + 1] = b64ctEncode6Std(s1);
            out[o + 2] = b64ctEncode6Std(s2);
        }
        o = o + 3;
        if (padded) {
            out[o] = '=' as u8;
            o = o + 1;
        }
    }

    return o;
}

pub fn encodeStd(src: []u8, out: []u8): usize {
    return b64ctEncodeCore(src, out, false, true);
}

pub fn encodeStdUnpadded(src: []u8, out: []u8): usize {
    return b64ctEncodeCore(src, out, false, false);
}

pub fn encodeUrl(src: []u8, out: []u8): usize {
    return b64ctEncodeCore(src, out, true, true);
}

pub fn encodeUrlUnpadded(src: []u8, out: []u8): usize {
    return b64ctEncodeCore(src, out, true, false);
}

fn b64ctZeroOut(out: []u8, n: usize) {
    let i: usize = 0;
    while (i < n && i < out.len) {
        out[i] = 0;
        i = i + 1;
    }
}

pub fn decodeStd(src: string, out: []u8): DecodeResult {
    if (src.len == 0) {
        return DecodeResult { len: 0, ok: true };
    }
    if ((src.len % 4) != 0) {
        return DecodeResult { len: 0, ok: false };
    }

    let pad: usize = 0;
    if (src[src.len - 1] == ('=' as u8)) {
        pad = pad + 1;
        if (src[src.len - 2] == ('=' as u8)) {
            pad = pad + 1;
        }
    }

    let full_blocks = (src.len / 4) - 1;
    let last_bytes: usize = (3 as usize) - pad;
    let out_len = full_blocks * 3 + last_bytes;
    if (out.len < out_len) {
        return DecodeResult { len: 0, ok: false };
    }

    let err: u32 = 0;
    let si: usize = 0;
    let oi: usize = 0;
    let b: usize = 0;
    while (b < full_blocks) {
        let v0 = b64ctDecode6Std(src[si]);
        let v1 = b64ctDecode6Std(src[si + 1]);
        let v2 = b64ctDecode6Std(src[si + 2]);
        let v3 = b64ctDecode6Std(src[si + 3]);
        err = err | v0 | v1 | v2 | v3;
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
        out[oi + 1] = (((v1 & 0x0F) << 4) | ((v2 & 0x3F) >> 2)) as u8;
        out[oi + 2] = (((v2 & 0x03) << 6) | (v3 & 0x3F)) as u8;
        si = si + 4;
        oi = oi + 3;
        b = b + 1;
    }

    // Final 4-char block
    if (pad == 0) {
        let v0 = b64ctDecode6Std(src[si]);
        let v1 = b64ctDecode6Std(src[si + 1]);
        let v2 = b64ctDecode6Std(src[si + 2]);
        let v3 = b64ctDecode6Std(src[si + 3]);
        err = err | v0 | v1 | v2 | v3;
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
        out[oi + 1] = (((v1 & 0x0F) << 4) | ((v2 & 0x3F) >> 2)) as u8;
        out[oi + 2] = (((v2 & 0x03) << 6) | (v3 & 0x3F)) as u8;
    } else if (pad == 1) {
        let v0 = b64ctDecode6Std(src[si]);
        let v1 = b64ctDecode6Std(src[si + 1]);
        let v2 = b64ctDecode6Std(src[si + 2]);
        err = err | v0 | v1 | v2;
        // Canonical RFC 4648 check: lowest 2 bits of v2 must be zero
        if ((v2 & 0x03) != 0) {
            err = err | 0x80;
        }
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
        out[oi + 1] = (((v1 & 0x0F) << 4) | ((v2 & 0x3F) >> 2)) as u8;
    } else {
        let v0 = b64ctDecode6Std(src[si]);
        let v1 = b64ctDecode6Std(src[si + 1]);
        err = err | v0 | v1;
        // Canonical RFC 4648 check: lowest 4 bits of v1 must be zero
        if ((v1 & 0x0F) != 0) {
            err = err | 0x80;
        }
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
    }

    if ((err & 0x80) != 0) {
        b64ctZeroOut(out, out_len);
        return DecodeResult { len: 0, ok: false };
    }
    return DecodeResult { len: out_len, ok: true };
}

pub fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult {
    if (src.len == 0) {
        return DecodeResult { len: 0, ok: true };
    }
    let rem = src.len % 4;
    if (rem == 1) {
        return DecodeResult { len: 0, ok: false };
    }

    let full_blocks = src.len / 4;
    let tail_bytes: usize = 0;
    if (rem == 2) {
        tail_bytes = 1;
    } else if (rem == 3) {
        tail_bytes = 2;
    }
    let out_len = full_blocks * 3 + tail_bytes;
    if (out.len < out_len) {
        return DecodeResult { len: 0, ok: false };
    }

    let err: u32 = 0;
    let si: usize = 0;
    let oi: usize = 0;
    let b: usize = 0;
    while (b < full_blocks) {
        let v0 = b64ctDecode6Url(src[si]);
        let v1 = b64ctDecode6Url(src[si + 1]);
        let v2 = b64ctDecode6Url(src[si + 2]);
        let v3 = b64ctDecode6Url(src[si + 3]);
        err = err | v0 | v1 | v2 | v3;
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
        out[oi + 1] = (((v1 & 0x0F) << 4) | ((v2 & 0x3F) >> 2)) as u8;
        out[oi + 2] = (((v2 & 0x03) << 6) | (v3 & 0x3F)) as u8;
        si = si + 4;
        oi = oi + 3;
        b = b + 1;
    }

    if (rem == 2) {
        let v0 = b64ctDecode6Url(src[si]);
        let v1 = b64ctDecode6Url(src[si + 1]);
        err = err | v0 | v1;
        if ((v1 & 0x0F) != 0) {
            err = err | 0x80;
        }
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
    } else if (rem == 3) {
        let v0 = b64ctDecode6Url(src[si]);
        let v1 = b64ctDecode6Url(src[si + 1]);
        let v2 = b64ctDecode6Url(src[si + 2]);
        err = err | v0 | v1 | v2;
        if ((v2 & 0x03) != 0) {
            err = err | 0x80;
        }
        out[oi] = (((v0 & 0x3F) << 2) | ((v1 & 0x3F) >> 4)) as u8;
        out[oi + 1] = (((v1 & 0x0F) << 4) | ((v2 & 0x3F) >> 2)) as u8;
    }

    if ((err & 0x80) != 0) {
        b64ctZeroOut(out, out_len);
        return DecodeResult { len: 0, ok: false };
    }
    return DecodeResult { len: out_len, ok: true };
}

export {
    DecodeResult,
    encodedLen,
    encodeStd,
    encodeStdUnpadded,
    encodeUrl,
    encodeUrlUnpadded,
    decodeStd,
    decodeUrlUnpadded
};
