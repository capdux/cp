// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Extended Binary Golay [24,12,8] & Perfect Golay [23,12,7] FEC Codec)

/// Extended Binary Golay [24, 12, 8] protocol constants.
pub const GOLAY24_DATA_BITS: u8 = 12;
pub const GOLAY24_CODE_BITS: u8 = 24;
pub const GOLAY24_MIN_DIST: u8 = 8;

/// Perfect Binary Golay [23, 12, 7] codeword bit length.
pub const GOLAY23_CODE_BITS: u8 = 23;

/// Decoding status for Golay [24, 12, 8] and [23, 12, 7] codewords.
pub enum GolayStatus: u8 {
    ok = 0,
    corrected = 1,
    uncorrectable = 2,
}

/// Decoded Golay codeword result.
pub struct GolayDecode {
    data: u16;
    codeword: u32;
    errors_corrected: u8;
    status: GolayStatus;
}

/// Symmetric self-inverse 12x12 parity submatrix B of G = [I_12 | B] (B = B^T, B^2 = I_12 mod 2).
/// Rows 0..10 have Hamming weight 7; row 11 has Hamming weight 11.
const GOLAY_B: [12]u16 = [
    0xA3B,
    0xD1D,
    0xE8E,
    0xB47,
    0xDA3,
    0xED1,
    0xF68,
    0xBB4,
    0x9DA,
    0x8ED,
    0xC76,
    0x7FF,
];

fn popcount32(x: u32): u8 {
    let v = x;
    let c: u8 = 0;
    while (v != 0) {
        v = v & (v - 1);
        c += 1;
    }
    return c;
}

fn popcount16(x: u16): u8 {
    return popcount32(x as u32);
}

fn mulB(vec12: u16): u16 {
    let v = vec12 & 0x0FFF;
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < 12) {
        if (((v >> (i as u4)) & 1) != 0) {
            acc = acc ^ GOLAY_B[i];
        }
        i += 1;
    }
    return acc;
}

/// Computes the 12-bit parity word for a 12-bit data payload (`data12 & 0xFFF`).
pub fn golay24Parity(data12: u16): u16 {
    return mulB(data12 & 0x0FFF);
}

/// Encodes a 12-bit data payload into a 24-bit Extended Golay [24, 12, 8] codeword:
/// `(data12 << 12) | parity12`.
pub fn golay24Encode(data12: u16): u32 {
    const d = data12 & 0x0FFF;
    const p = mulB(d);
    return ((d as u32) << 12) | (p as u32);
}

/// Decodes a 24-bit Extended Golay [24, 12, 8] codeword using 4-step IMLD syndrome decoding.
/// - Corrects any 0, 1, 2, or 3 bit errors (`GolayStatus.ok` or `GolayStatus.corrected`).
/// - Detects 4-bit errors as `GolayStatus.uncorrectable`.
pub fn golay24Decode(codeword24: u32): GolayDecode {
    const w = codeword24 & 0x00FFFFFF;
    const u: u16 = ((w >> 12) & 0x0FFF) as u16;
    const p: u16 = (w & 0x0FFF) as u16;

    // Step 1: s1 = p ^ (u * B). If weight(s1) <= 3, all errors are in parity bits.
    const s1 = p ^ mulB(u);
    const w1 = popcount16(s1);
    if (w1 == 0) {
        return GolayDecode {
            data: u,
            codeword: w,
            errors_corrected: 0,
            status: GolayStatus.ok,
        };
    }
    if (w1 <= 3) {
        const corr_p = p ^ s1;
        return GolayDecode {
            data: u,
            codeword: ((u as u32) << 12) | (corr_p as u32),
            errors_corrected: w1,
            status: GolayStatus.corrected,
        };
    }

    // Step 2: Check 1 error in data bit i and <= 2 errors in parity bits.
    let i: usize = 0;
    while (i < 12) {
        const t = s1 ^ GOLAY_B[i];
        const wt = popcount16(t);
        if (wt <= 2) {
            const err_u: u16 = (1 as u16) << (i as u4);
            const corr_u = u ^ err_u;
            const corr_p = p ^ t;
            return GolayDecode {
                data: corr_u,
                codeword: ((corr_u as u32) << 12) | (corr_p as u32),
                errors_corrected: 1 + wt,
                status: GolayStatus.corrected,
            };
        }
        i += 1;
    }

    // Step 3: s2 = s1 * B (= u ^ (p * B)). If weight(s2) <= 3, all errors are in data bits.
    const s2 = mulB(s1);
    const w2 = popcount16(s2);
    if (w2 <= 3) {
        const corr_u = u ^ s2;
        return GolayDecode {
            data: corr_u,
            codeword: ((corr_u as u32) << 12) | (p as u32),
            errors_corrected: w2,
            status: GolayStatus.corrected,
        };
    }

    // Step 4: Check 1 error in parity bit j and <= 2 errors in data bits.
    let j: usize = 0;
    while (j < 12) {
        const t = s2 ^ GOLAY_B[j];
        const wt = popcount16(t);
        if (wt <= 2) {
            const err_p: u16 = (1 as u16) << (j as u4);
            const corr_u = u ^ t;
            const corr_p = p ^ err_p;
            return GolayDecode {
                data: corr_u,
                codeword: ((corr_u as u32) << 12) | (corr_p as u32),
                errors_corrected: 1 + wt,
                status: GolayStatus.corrected,
            };
        }
        j += 1;
    }

    // Uncorrectable (distance >= 4 from all valid codewords, e.g. 4-bit error).
    return GolayDecode {
        data: u,
        codeword: w,
        errors_corrected: 0,
        status: GolayStatus.uncorrectable,
    };
}

/// Encodes a 12-bit data payload into a 23-bit Perfect Golay [23, 12, 7] codeword
/// by puncturing bit 0 of the Extended Golay [24, 12, 8] codeword.
pub fn golay23Encode(data12: u16): u32 {
    return golay24Encode(data12) >> 1;
}

/// Decodes a 23-bit Perfect Golay [23, 12, 7] codeword by appending an odd-parity bit
/// and delegating to `golay24Decode`. Uniquely corrects any 0, 1, 2, or 3 bit errors.
pub fn golay23Decode(codeword23: u32): GolayDecode {
    const w23 = codeword23 & 0x007FFFFF;
    const odd_bit: u32 = ((popcount32(w23) & 1) ^ 1) as u32;
    const cand24 = (w23 << 1) | odd_bit;
    const dec24 = golay24Decode(cand24);
    const corr23 = (dec24.codeword >> 1) & 0x007FFFFF;
    const errs = popcount32(w23 ^ corr23);
    const st = if (errs == 0) GolayStatus.ok else GolayStatus.corrected;
    return GolayDecode {
        data: dec24.data,
        codeword: corr23,
        errors_corrected: errs,
        status: st,
    };
}

fn writeU24BE(dst: []u8, pos: usize, val: u32) {
    dst[pos] = ((val >> 16) & 0xFF) as u8;
    dst[pos + 1] = ((val >> 8) & 0xFF) as u8;
    dst[pos + 2] = (val & 0xFF) as u8;
}

fn readU24BE(src: []const u8, pos: usize): u32 {
    return ((src[pos] as u32) << 16) | ((src[pos + 1] as u32) << 8) | (src[pos + 2] as u32);
}

/// Encodes arbitrary byte stream `src` into Extended Golay [24, 12, 8] codewords in `dst`.
/// - Every 3 input bytes (24 bits) are split into two 12-bit payloads -> two 24-bit codewords (6 bytes).
/// - A trailing 1-byte remainder encodes as 1 codeword (3 bytes, zero-padded low 4 bits).
/// - A trailing 2-byte remainder encodes as 2 codewords (6 bytes, zero-padded low 8 bits).
/// Returns bytes written to `dst`, or `null` if `dst` is too small.
pub fn golay24EncodeBytes(src: []const u8, dst: []u8): usize? {
    const full_groups = src.len / 3;
    const rem = src.len % 3;
    let needed = full_groups * 6;
    if (rem == 1) {
        needed += 3;
    } else if (rem == 2) {
        needed += 6;
    }
    if (dst.len < needed) {
        return null;
    }

    let in_pos: usize = 0;
    let out_pos: usize = 0;
    let g: usize = 0;
    while (g < full_groups) {
        const b0 = src[in_pos] as u16;
        const b1 = src[in_pos + 1] as u16;
        const b2 = src[in_pos + 2] as u16;
        const d0 = (b0 << 4) | (b1 >> 4);
        const d1 = ((b1 & 0x0F) << 8) | b2;
        writeU24BE(dst, out_pos, golay24Encode(d0));
        writeU24BE(dst, out_pos + 3, golay24Encode(d1));
        in_pos += 3;
        out_pos += 6;
        g += 1;
    }

    if (rem == 1) {
        const b0 = src[in_pos] as u16;
        const d0 = b0 << 4;
        writeU24BE(dst, out_pos, golay24Encode(d0));
        out_pos += 3;
    } else if (rem == 2) {
        const b0 = src[in_pos] as u16;
        const b1 = src[in_pos + 1] as u16;
        const d0 = (b0 << 4) | (b1 >> 4);
        const d1 = (b1 & 0x0F) << 8;
        writeU24BE(dst, out_pos, golay24Encode(d0));
        writeU24BE(dst, out_pos + 3, golay24Encode(d1));
        out_pos += 6;
    }

    return out_pos;
}

/// Decodes a Golay [24, 12, 8] encoded byte stream `src` into `dst`, correcting up to 3 bit errors per 24-bit codeword.
/// Returns bytes written to `dst`, or `null` if `src.len % 3 != 0`, `dst` is too small, or any codeword is uncorrectable.
pub fn golay24DecodeBytes(src: []const u8, dst: []u8): usize? {
    if ((src.len % 3) != 0) {
        return null;
    }
    const full_pairs = src.len / 6;
    const has_tail = (src.len % 6) == 3;
    const needed = full_pairs * 3 + (if (has_tail) 1 as usize else 0 as usize);
    if (dst.len < needed) {
        return null;
    }

    let in_pos: usize = 0;
    let out_pos: usize = 0;
    let p: usize = 0;
    while (p < full_pairs) {
        const r0 = golay24Decode(readU24BE(src, in_pos));
        if (r0.status == GolayStatus.uncorrectable) {
            return null;
        }
        const r1 = golay24Decode(readU24BE(src, in_pos + 3));
        if (r1.status == GolayStatus.uncorrectable) {
            return null;
        }
        const d0 = r0.data;
        const d1 = r1.data;
        dst[out_pos] = ((d0 >> 4) & 0xFF) as u8;
        dst[out_pos + 1] = (((d0 & 0x0F) << 4) | ((d1 >> 8) & 0x0F)) as u8;
        dst[out_pos + 2] = (d1 & 0xFF) as u8;
        in_pos += 6;
        out_pos += 3;
        p += 1;
    }

    if (has_tail) {
        const r0 = golay24Decode(readU24BE(src, in_pos));
        if (r0.status == GolayStatus.uncorrectable) {
            return null;
        }
        dst[out_pos] = ((r0.data >> 4) & 0xFF) as u8;
        out_pos += 1;
    }

    return out_pos;
}
