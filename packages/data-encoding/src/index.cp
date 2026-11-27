// @cap/data-encoding — Zero-allocation strict RFC 4648 Base16/Hex, Base32, Base64/Base64URL codec for CP
// Clean-room MIT implementation inspired by Rust `data-encoding` v2 (800M+ downloads).
// Enforces canonical RFC 4648 §3.5 non-zero trailing-bit rejection and precise error position reporting.

pub struct DecodeResult {
    pub ok: bool,
    pub len: usize,
    pub error_pos: usize,

    pub static fn success(n: usize) -> DecodeResult {
        return DecodeResult{
            ok: true,
            len: n,
            error_pos: 0 as usize,
        };
    }

    pub static fn fail(pos: usize) -> DecodeResult {
        return DecodeResult{
            ok: false,
            len: 0 as usize,
            error_pos: pos,
        };
    }
}

fn dencHexDigit(v: u8, upper: bool) -> u8 {
    if (v < (10 as u8)) {
        return (48 as u8) + v;
    }
    if (upper) {
        return (65 as u8) + (v - (10 as u8));
    }
    return (97 as u8) + (v - (10 as u8));
}

fn dencHexVal(c: u8) -> u8 {
    if (c >= (48 as u8) && c <= (57 as u8)) {
        return c - (48 as u8);
    }
    if (c >= (65 as u8) && c <= (70 as u8)) {
        return (c - (65 as u8)) + (10 as u8);
    }
    if (c >= (97 as u8) && c <= (102 as u8)) {
        return (c - (97 as u8)) + (10 as u8);
    }
    return 0xFF as u8;
}

fn dencB32Char(v: u8) -> u8 {
    if (v < (26 as u8)) {
        return (65 as u8) + v;
    }
    return (50 as u8) + (v - (26 as u8));
}

fn dencB32Val(c: u8) -> u8 {
    if (c >= (65 as u8) && c <= (90 as u8)) {
        return c - (65 as u8);
    }
    if (c >= (50 as u8) && c <= (55 as u8)) {
        return (c - (50 as u8)) + (26 as u8);
    }
    return 0xFF as u8;
}

fn dencB64Char(v: u8, url_safe: bool) -> u8 {
    if (v < (26 as u8)) {
        return (65 as u8) + v;
    }
    if (v < (52 as u8)) {
        return (97 as u8) + (v - (26 as u8));
    }
    if (v < (62 as u8)) {
        return (48 as u8) + (v - (52 as u8));
    }
    if (v == (62 as u8)) {
        if (url_safe) {
            return 45 as u8; // '-'
        }
        return 43 as u8; // '+'
    }
    if (url_safe) {
        return 95 as u8; // '_'
    }
    return 47 as u8; // '/'
}

fn dencB64Val(c: u8, url_safe: bool) -> u8 {
    if (c >= (65 as u8) && c <= (90 as u8)) {
        return c - (65 as u8);
    }
    if (c >= (97 as u8) && c <= (122 as u8)) {
        return (c - (97 as u8)) + (26 as u8);
    }
    if (c >= (48 as u8) && c <= (57 as u8)) {
        return (c - (48 as u8)) + (52 as u8);
    }
    if (url_safe) {
        if (c == (45 as u8)) {
            return 62 as u8;
        }
        if (c == (95 as u8)) {
            return 63 as u8;
        }
        return 0xFF as u8;
    }
    if (c == (43 as u8)) {
        return 62 as u8;
    }
    if (c == (47 as u8)) {
        return 63 as u8;
    }
    return 0xFF as u8;
}

fn dencHexEncodeImpl(src: []const u8, upper: bool, out: []u8) -> usize {
    let need = src.len * (2 as usize);
    if (out.len < need) {
        return 0 as usize;
    }
    let i: usize = 0;
    while (i < src.len) {
        let b = src[i];
        out[i * (2 as usize)] = dencHexDigit(b / (16 as u8), upper);
        out[i * (2 as usize) + (1 as usize)] = dencHexDigit(b & (0x0F as u8), upper);
        i += 1;
    }
    return need;
}

/// Computes exact encoded length for Base64 / Base64URL.
pub fn base64EncodeLen(n: usize, pad: bool) -> usize {
    if (n == (0 as usize)) {
        return 0 as usize;
    }
    if (pad) {
        return ((n + (2 as usize)) / (3 as usize)) * (4 as usize);
    }
    let full = (n / (3 as usize)) * (4 as usize);
    let rem = n % (3 as usize);
    if (rem == (0 as usize)) {
        return full;
    }
    if (rem == (1 as usize)) {
        return full + (2 as usize);
    }
    return full + (3 as usize);
}

/// Computes exact encoded length for RFC 4648 Base32.
pub fn base32EncodeLen(n: usize, pad: bool) -> usize {
    if (n == (0 as usize)) {
        return 0 as usize;
    }
    if (pad) {
        return ((n + (4 as usize)) / (5 as usize)) * (8 as usize);
    }
    let full = (n / (5 as usize)) * (8 as usize);
    let rem = n % (5 as usize);
    if (rem == (0 as usize)) {
        return full;
    }
    if (rem == (1 as usize)) {
        return full + (2 as usize);
    }
    if (rem == (2 as usize)) {
        return full + (4 as usize);
    }
    if (rem == (3 as usize)) {
        return full + (5 as usize);
    }
    return full + (7 as usize);
}

/// Encodes `src` as lowercase hexadecimal into `out`. Returns bytes written (0 if `out` too small).
pub fn hexLowerEncode(src: []const u8, out: []u8) -> usize {
    return dencHexEncodeImpl(src, false, out);
}

/// Encodes `src` as uppercase hexadecimal into `out`. Returns bytes written (0 if `out` too small).
pub fn hexUpperEncode(src: []const u8, out: []u8) -> usize {
    return dencHexEncodeImpl(src, true, out);
}

/// Decodes hexadecimal (accepting both upper & lower case) into `out`.
pub fn hexDecode(src: []const u8, out: []u8) -> DecodeResult {
    if (src.len == (0 as usize)) {
        return DecodeResult.success(0 as usize);
    }
    if ((src.len % (2 as usize)) != (0 as usize)) {
        return DecodeResult.fail(src.len - (1 as usize));
    }
    let out_len = src.len / (2 as usize);
    if (out.len < out_len) {
        return DecodeResult.fail(0 as usize);
    }
    let i: usize = 0;
    while (i < out_len) {
        let hi = dencHexVal(src[i * (2 as usize)]);
        if (hi == (0xFF as u8)) {
            return DecodeResult.fail(i * (2 as usize));
        }
        let lo = dencHexVal(src[i * (2 as usize) + (1 as usize)]);
        if (lo == (0xFF as u8)) {
            return DecodeResult.fail(i * (2 as usize) + (1 as usize));
        }
        out[i] = (hi * (16 as u8)) | lo;
        i += 1;
    }
    return DecodeResult.success(out_len);
}

/// Encodes `src` using RFC 4648 Base32 alphabet (`A-Z`, `2-7`) with optional `=` padding.
pub fn base32Encode(src: []const u8, pad: bool, out: []u8) -> usize {
    let need = base32EncodeLen(src.len, pad);
    if (out.len < need) {
        return 0 as usize;
    }
    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + (5 as usize) <= src.len) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        let b2 = src[in_i + (2 as usize)] as u32;
        let b3 = src[in_i + (3 as usize)] as u32;
        let b4 = src[in_i + (4 as usize)] as u32;

        out[out_i] = dencB32Char((b0 / (8 as u32)) as u8);
        out[out_i + (1 as usize)] = dencB32Char((((b0 & (7 as u32)) * (4 as u32)) | (b1 / (64 as u32))) as u8);
        out[out_i + (2 as usize)] = dencB32Char(((b1 / (2 as u32)) & (31 as u32)) as u8);
        out[out_i + (3 as usize)] = dencB32Char((((b1 & (1 as u32)) * (16 as u32)) | (b2 / (16 as u32))) as u8);
        out[out_i + (4 as usize)] = dencB32Char((((b2 & (15 as u32)) * (2 as u32)) | (b3 / (128 as u32))) as u8);
        out[out_i + (5 as usize)] = dencB32Char(((b3 / (4 as u32)) & (31 as u32)) as u8);
        out[out_i + (6 as usize)] = dencB32Char((((b3 & (3 as u32)) * (8 as u32)) | (b4 / (32 as u32))) as u8);
        out[out_i + (7 as usize)] = dencB32Char((b4 & (31 as u32)) as u8);

        in_i += 5;
        out_i += 8;
    }

    let rem = src.len - in_i;
    if (rem == (1 as usize)) {
        let b0 = src[in_i] as u32;
        out[out_i] = dencB32Char((b0 / (8 as u32)) as u8);
        out[out_i + (1 as usize)] = dencB32Char(((b0 & (7 as u32)) * (4 as u32)) as u8);
        out_i += 2;
        if (pad) {
            let p: usize = 0;
            while (p < (6 as usize)) {
                out[out_i] = 61 as u8; // '='
                out_i += 1;
                p += 1;
            }
        }
    } else if (rem == (2 as usize)) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        out[out_i] = dencB32Char((b0 / (8 as u32)) as u8);
        out[out_i + (1 as usize)] = dencB32Char((((b0 & (7 as u32)) * (4 as u32)) | (b1 / (64 as u32))) as u8);
        out[out_i + (2 as usize)] = dencB32Char(((b1 / (2 as u32)) & (31 as u32)) as u8);
        out[out_i + (3 as usize)] = dencB32Char(((b1 & (1 as u32)) * (16 as u32)) as u8);
        out_i += 4;
        if (pad) {
            let p: usize = 0;
            while (p < (4 as usize)) {
                out[out_i] = 61 as u8;
                out_i += 1;
                p += 1;
            }
        }
    } else if (rem == (3 as usize)) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        let b2 = src[in_i + (2 as usize)] as u32;
        out[out_i] = dencB32Char((b0 / (8 as u32)) as u8);
        out[out_i + (1 as usize)] = dencB32Char((((b0 & (7 as u32)) * (4 as u32)) | (b1 / (64 as u32))) as u8);
        out[out_i + (2 as usize)] = dencB32Char(((b1 / (2 as u32)) & (31 as u32)) as u8);
        out[out_i + (3 as usize)] = dencB32Char((((b1 & (1 as u32)) * (16 as u32)) | (b2 / (16 as u32))) as u8);
        out[out_i + (4 as usize)] = dencB32Char(((b2 & (15 as u32)) * (2 as u32)) as u8);
        out_i += 5;
        if (pad) {
            let p: usize = 0;
            while (p < (3 as usize)) {
                out[out_i] = 61 as u8;
                out_i += 1;
                p += 1;
            }
        }
    } else if (rem == (4 as usize)) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        let b2 = src[in_i + (2 as usize)] as u32;
        let b3 = src[in_i + (3 as usize)] as u32;
        out[out_i] = dencB32Char((b0 / (8 as u32)) as u8);
        out[out_i + (1 as usize)] = dencB32Char((((b0 & (7 as u32)) * (4 as u32)) | (b1 / (64 as u32))) as u8);
        out[out_i + (2 as usize)] = dencB32Char(((b1 / (2 as u32)) & (31 as u32)) as u8);
        out[out_i + (3 as usize)] = dencB32Char((((b1 & (1 as u32)) * (16 as u32)) | (b2 / (16 as u32))) as u8);
        out[out_i + (4 as usize)] = dencB32Char((((b2 & (15 as u32)) * (2 as u32)) | (b3 / (128 as u32))) as u8);
        out[out_i + (5 as usize)] = dencB32Char(((b3 / (4 as u32)) & (31 as u32)) as u8);
        out[out_i + (6 as usize)] = dencB32Char(((b3 & (3 as u32)) * (8 as u32)) as u8);
        out_i += 7;
        if (pad) {
            out[out_i] = 61 as u8;
            out_i += 1;
        }
    }

    return out_i;
}

/// Decodes RFC 4648 Base32 (padded or unpadded) with strict canonical trailing-bit verification.
pub fn base32Decode(src: []const u8, out: []u8) -> DecodeResult {
    if (src.len == (0 as usize)) {
        return DecodeResult.success(0 as usize);
    }

    let data_len = src.len;
    let pad_count: usize = 0;
    while (data_len > (0 as usize) && src[data_len - (1 as usize)] == (61 as u8)) {
        data_len -= 1;
        pad_count += 1;
    }

    if (pad_count > (0 as usize)) {
        if ((src.len % (8 as usize)) != (0 as usize)) {
            return DecodeResult.fail(data_len);
        }
        if (pad_count != (1 as usize) && pad_count != (3 as usize) && pad_count != (4 as usize) && pad_count != (6 as usize)) {
            return DecodeResult.fail(data_len);
        }
    }

    let rem = data_len % (8 as usize);
    if (rem == (1 as usize) || rem == (3 as usize) || rem == (6 as usize)) {
        return DecodeResult.fail(data_len - (1 as usize));
    }

    let full_blocks = data_len / (8 as usize);
    let tail_bytes: usize = 0;
    if (rem == (2 as usize)) {
        tail_bytes = 1 as usize;
    } else if (rem == (4 as usize)) {
        tail_bytes = 2 as usize;
    } else if (rem == (5 as usize)) {
        tail_bytes = 3 as usize;
    } else if (rem == (7 as usize)) {
        tail_bytes = 4 as usize;
    }
    let total_out = full_blocks * (5 as usize) + tail_bytes;
    if (out.len < total_out) {
        return DecodeResult.fail(0 as usize);
    }

    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + (8 as usize) <= data_len) {
        let v: [8]u32 = undefined;
        let k: usize = 0;
        while (k < (8 as usize)) {
            let d = dencB32Val(src[in_i + k]);
            if (d == (0xFF as u8)) {
                return DecodeResult.fail(in_i + k);
            }
            v[k] = d as u32;
            k += 1;
        }
        out[out_i] = ((v[0] * (8 as u32)) | (v[1] / (4 as u32))) as u8;
        out[out_i + (1 as usize)] = (((v[1] & (3 as u32)) * (64 as u32)) | (v[2] * (2 as u32)) | (v[3] / (16 as u32))) as u8;
        out[out_i + (2 as usize)] = (((v[3] & (15 as u32)) * (16 as u32)) | (v[4] / (2 as u32))) as u8;
        out[out_i + (3 as usize)] = (((v[4] & (1 as u32)) * (128 as u32)) | (v[5] * (4 as u32)) | (v[6] / (8 as u32))) as u8;
        out[out_i + (4 as usize)] = (((v[6] & (7 as u32)) * (32 as u32)) | v[7]) as u8;

        in_i += 8;
        out_i += 5;
    }

    if (rem > (0 as usize)) {
        let v: [7]u32 = undefined;
        let k: usize = 0;
        while (k < rem) {
            let d = dencB32Val(src[in_i + k]);
            if (d == (0xFF as u8)) {
                return DecodeResult.fail(in_i + k);
            }
            v[k] = d as u32;
            k += 1;
        }

        if (rem == (2 as usize)) {
            if ((v[1] & (3 as u32)) != (0 as u32)) {
                return DecodeResult.fail(in_i + (1 as usize));
            }
            out[out_i] = ((v[0] * (8 as u32)) | (v[1] / (4 as u32))) as u8;
            out_i += 1;
        } else if (rem == (4 as usize)) {
            if ((v[3] & (15 as u32)) != (0 as u32)) {
                return DecodeResult.fail(in_i + (3 as usize));
            }
            out[out_i] = ((v[0] * (8 as u32)) | (v[1] / (4 as u32))) as u8;
            out[out_i + (1 as usize)] = (((v[1] & (3 as u32)) * (64 as u32)) | (v[2] * (2 as u32)) | (v[3] / (16 as u32))) as u8;
            out_i += 2;
        } else if (rem == (5 as usize)) {
            if ((v[4] & (1 as u32)) != (0 as u32)) {
                return DecodeResult.fail(in_i + (4 as usize));
            }
            out[out_i] = ((v[0] * (8 as u32)) | (v[1] / (4 as u32))) as u8;
            out[out_i + (1 as usize)] = (((v[1] & (3 as u32)) * (64 as u32)) | (v[2] * (2 as u32)) | (v[3] / (16 as u32))) as u8;
            out[out_i + (2 as usize)] = (((v[3] & (15 as u32)) * (16 as u32)) | (v[4] / (2 as u32))) as u8;
            out_i += 3;
        } else if (rem == (7 as usize)) {
            if ((v[6] & (7 as u32)) != (0 as u32)) {
                return DecodeResult.fail(in_i + (6 as usize));
            }
            out[out_i] = ((v[0] * (8 as u32)) | (v[1] / (4 as u32))) as u8;
            out[out_i + (1 as usize)] = (((v[1] & (3 as u32)) * (64 as u32)) | (v[2] * (2 as u32)) | (v[3] / (16 as u32))) as u8;
            out[out_i + (2 as usize)] = (((v[3] & (15 as u32)) * (16 as u32)) | (v[4] / (2 as u32))) as u8;
            out[out_i + (3 as usize)] = (((v[4] & (1 as u32)) * (128 as u32)) | (v[5] * (4 as u32)) | (v[6] / (8 as u32))) as u8;
            out_i += 4;
        }
    }

    return DecodeResult.success(out_i);
}

/// Encodes `src` using RFC 4648 Base64 or Base64URL with optional `=` padding.
pub fn base64Encode(src: []const u8, url_safe: bool, pad: bool, out: []u8) -> usize {
    let need = base64EncodeLen(src.len, pad);
    if (out.len < need) {
        return 0 as usize;
    }
    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + (3 as usize) <= src.len) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        let b2 = src[in_i + (2 as usize)] as u32;

        out[out_i] = dencB64Char((b0 / (4 as u32)) as u8, url_safe);
        out[out_i + (1 as usize)] = dencB64Char((((b0 & (3 as u32)) * (16 as u32)) | (b1 / (16 as u32))) as u8, url_safe);
        out[out_i + (2 as usize)] = dencB64Char((((b1 & (15 as u32)) * (4 as u32)) | (b2 / (64 as u32))) as u8, url_safe);
        out[out_i + (3 as usize)] = dencB64Char((b2 & (63 as u32)) as u8, url_safe);

        in_i += 3;
        out_i += 4;
    }

    let rem = src.len - in_i;
    if (rem == (1 as usize)) {
        let b0 = src[in_i] as u32;
        out[out_i] = dencB64Char((b0 / (4 as u32)) as u8, url_safe);
        out[out_i + (1 as usize)] = dencB64Char(((b0 & (3 as u32)) * (16 as u32)) as u8, url_safe);
        out_i += 2;
        if (pad) {
            out[out_i] = 61 as u8;
            out[out_i + (1 as usize)] = 61 as u8;
            out_i += 2;
        }
    } else if (rem == (2 as usize)) {
        let b0 = src[in_i] as u32;
        let b1 = src[in_i + (1 as usize)] as u32;
        out[out_i] = dencB64Char((b0 / (4 as u32)) as u8, url_safe);
        out[out_i + (1 as usize)] = dencB64Char((((b0 & (3 as u32)) * (16 as u32)) | (b1 / (16 as u32))) as u8, url_safe);
        out[out_i + (2 as usize)] = dencB64Char(((b1 & (15 as u32)) * (4 as u32)) as u8, url_safe);
        out_i += 3;
        if (pad) {
            out[out_i] = 61 as u8;
            out_i += 1;
        }
    }
    return out_i;
}

/// Decodes RFC 4648 Base64 or Base64URL (padded or unpadded) with strict canonical trailing-bit verification.
pub fn base64Decode(src: []const u8, url_safe: bool, out: []u8) -> DecodeResult {
    if (src.len == (0 as usize)) {
        return DecodeResult.success(0 as usize);
    }

    let data_len = src.len;
    let pad_count: usize = 0;
    while (data_len > (0 as usize) && src[data_len - (1 as usize)] == (61 as u8)) {
        data_len -= 1;
        pad_count += 1;
    }

    if (pad_count > (0 as usize)) {
        if ((src.len % (4 as usize)) != (0 as usize)) {
            return DecodeResult.fail(data_len);
        }
        if (pad_count > (2 as usize)) {
            return DecodeResult.fail(data_len);
        }
    }

    let rem = data_len % (4 as usize);
    if (rem == (1 as usize)) {
        return DecodeResult.fail(data_len - (1 as usize));
    }

    let full_blocks = data_len / (4 as usize);
    let tail_bytes: usize = 0;
    if (rem == (2 as usize)) {
        tail_bytes = 1 as usize;
    } else if (rem == (3 as usize)) {
        tail_bytes = 2 as usize;
    }
    let total_out = full_blocks * (3 as usize) + tail_bytes;
    if (out.len < total_out) {
        return DecodeResult.fail(0 as usize);
    }

    let in_i: usize = 0;
    let out_i: usize = 0;
    while (in_i + (4 as usize) <= data_len) {
        let d0 = dencB64Val(src[in_i], url_safe);
        if (d0 == (0xFF as u8)) {
            return DecodeResult.fail(in_i);
        }
        let d1 = dencB64Val(src[in_i + (1 as usize)], url_safe);
        if (d1 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (1 as usize));
        }
        let d2 = dencB64Val(src[in_i + (2 as usize)], url_safe);
        if (d2 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (2 as usize));
        }
        let d3 = dencB64Val(src[in_i + (3 as usize)], url_safe);
        if (d3 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (3 as usize));
        }

        let v0 = d0 as u32;
        let v1 = d1 as u32;
        let v2 = d2 as u32;
        let v3 = d3 as u32;

        out[out_i] = ((v0 * (4 as u32)) | (v1 / (16 as u32))) as u8;
        out[out_i + (1 as usize)] = (((v1 & (15 as u32)) * (16 as u32)) | (v2 / (4 as u32))) as u8;
        out[out_i + (2 as usize)] = (((v2 & (3 as u32)) * (64 as u32)) | v3) as u8;

        in_i += 4;
        out_i += 3;
    }

    if (rem == (2 as usize)) {
        let d0 = dencB64Val(src[in_i], url_safe);
        if (d0 == (0xFF as u8)) {
            return DecodeResult.fail(in_i);
        }
        let d1 = dencB64Val(src[in_i + (1 as usize)], url_safe);
        if (d1 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (1 as usize));
        }
        let v0 = d0 as u32;
        let v1 = d1 as u32;
        if ((v1 & (15 as u32)) != (0 as u32)) {
            return DecodeResult.fail(in_i + (1 as usize));
        }
        out[out_i] = ((v0 * (4 as u32)) | (v1 / (16 as u32))) as u8;
        out_i += 1;
    } else if (rem == (3 as usize)) {
        let d0 = dencB64Val(src[in_i], url_safe);
        if (d0 == (0xFF as u8)) {
            return DecodeResult.fail(in_i);
        }
        let d1 = dencB64Val(src[in_i + (1 as usize)], url_safe);
        if (d1 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (1 as usize));
        }
        let d2 = dencB64Val(src[in_i + (2 as usize)], url_safe);
        if (d2 == (0xFF as u8)) {
            return DecodeResult.fail(in_i + (2 as usize));
        }
        let v0 = d0 as u32;
        let v1 = d1 as u32;
        let v2 = d2 as u32;
        if ((v2 & (3 as u32)) != (0 as u32)) {
            return DecodeResult.fail(in_i + (2 as usize));
        }
        out[out_i] = ((v0 * (4 as u32)) | (v1 / (16 as u32))) as u8;
        out[out_i + (1 as usize)] = (((v1 & (15 as u32)) * (16 as u32)) | (v2 / (4 as u32))) as u8;
        out_i += 2;
    }

    return DecodeResult.success(out_i);
}
