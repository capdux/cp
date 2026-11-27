// @cap/zbase32 — Zero-allocation Zooko Human-Oriented z-base-32 Codec
// Clean-room MIT implementation in pure CP.
// Alphabet: "ybndrfg8ejkmcpqxot1uwisza345h769" (unpadded, big-endian 5-bit groups).
// Accelerated with comptime 256-entry LUT, 16-lane @Vector(16, u8) SIMD validation,
// and 5-byte/8-char unrolled block transcoding.

type Zb32Vec16 = @Vector(16, u8);

const ALPHABET: string = "ybndrfg8ejkmcpqxot1uwisza345h769";

fn zb32BuildDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let i: usize = 0;
    while (i < 32) {
        const c = ALPHABET[i] as usize;
        lut[c] = i as u8;
        if (c >= 97 && c <= 122) {
            lut[c - 32] = i as u8;
        }
        i += 1;
    }
    return lut;
}

const ZB32_DEC_LUT: [256]u8 = zb32BuildDecLut();

/// Returns the exact unpadded z-base-32 encoded character length for `byte_len` bytes:
/// `(byte_len * 8 + 4) / 5` (0 for `byte_len == 0`).
pub fn encodedLen(byte_len: usize) -> usize {
    if (byte_len == 0) {
        return 0;
    }
    return (byte_len * 8 + 4) / 5;
}

/// Encodes `data` into unpadded z-base-32 characters in `out`.
/// Returns bytes written, or `0` if `data` is empty or `out` is too small.
pub fn encode(data: []const u8, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    const needed = encodedLen(data.len);
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    let i: usize = 0;

    // Fast path: 5 bytes (40 bits) -> 8 chars per iteration
    while (i + 5 <= data.len) {
        const b0 = data[i] as u64;
        const b1 = data[i + 1] as u64;
        const b2 = data[i + 2] as u64;
        const b3 = data[i + 3] as u64;
        const b4 = data[i + 4] as u64;
        const blk: u64 = (b0 << 32) | (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
        out[w] = ALPHABET[((blk >> 35) & 0x1F) as usize];
        out[w + 1] = ALPHABET[((blk >> 30) & 0x1F) as usize];
        out[w + 2] = ALPHABET[((blk >> 25) & 0x1F) as usize];
        out[w + 3] = ALPHABET[((blk >> 20) & 0x1F) as usize];
        out[w + 4] = ALPHABET[((blk >> 15) & 0x1F) as usize];
        out[w + 5] = ALPHABET[((blk >> 10) & 0x1F) as usize];
        out[w + 6] = ALPHABET[((blk >> 5) & 0x1F) as usize];
        out[w + 7] = ALPHABET[(blk & 0x1F) as usize];
        w += 8;
        i += 5;
    }

    let acc: u32 = 0;
    let bits: u32 = 0;
    while (i < data.len) {
        acc = (acc << 8) | (data[i] as u32);
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            const idx = ((acc >> (bits as u5)) & 0x1F) as usize;
            out[w] = ALPHABET[idx];
            w += 1;
        }
        i += 1;
    }

    if (bits > 0) {
        const idx = ((acc << ((5 - bits) as u5)) & 0x1F) as usize;
        out[w] = ALPHABET[idx];
        w += 1;
    }

    return w;
}

/// Decodes a z-base-32 string (case-insensitive) into raw bytes in `out`.
/// Returns bytes written, or `-1` on invalid character or insufficient buffer.
pub fn decode(input: string, out: []u8) -> i64 {
    if (input.len == 0) {
        return 0;
    }
    const out_needed = (input.len * 5) / 8;
    if (out_needed > out.len) {
        return -1;
    }

    let w: usize = 0;
    let i: usize = 0;

    // Fast path: 8 chars (40 bits) -> 5 bytes per iteration
    while (i + 8 <= input.len) {
        const v0 = ZB32_DEC_LUT[input[i] as usize];
        const v1 = ZB32_DEC_LUT[input[i + 1] as usize];
        const v2 = ZB32_DEC_LUT[input[i + 2] as usize];
        const v3 = ZB32_DEC_LUT[input[i + 3] as usize];
        const v4 = ZB32_DEC_LUT[input[i + 4] as usize];
        const v5 = ZB32_DEC_LUT[input[i + 5] as usize];
        const v6 = ZB32_DEC_LUT[input[i + 6] as usize];
        const v7 = ZB32_DEC_LUT[input[i + 7] as usize];
        if (((v0 | v1 | v2 | v3 | v4 | v5 | v6 | v7) & 0xE0) != 0) {
            return -1;
        }
        const blk: u64 = ((v0 as u64) << 35) |
            ((v1 as u64) << 30) |
            ((v2 as u64) << 25) |
            ((v3 as u64) << 20) |
            ((v4 as u64) << 15) |
            ((v5 as u64) << 10) |
            ((v6 as u64) << 5) |
            (v7 as u64);
        out[w] = ((blk >> 32) & 0xFF) as u8;
        out[w + 1] = ((blk >> 24) & 0xFF) as u8;
        out[w + 2] = ((blk >> 16) & 0xFF) as u8;
        out[w + 3] = ((blk >> 8) & 0xFF) as u8;
        out[w + 4] = (blk & 0xFF) as u8;
        w += 5;
        i += 8;
    }

    let acc: u32 = 0;
    let bits: u32 = 0;
    while (i < input.len) {
        const val = ZB32_DEC_LUT[input[i] as usize];
        if (val >= 32) {
            return -1;
        }
        acc = (acc << 5) | (val as u32);
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out[w] = ((acc >> (bits as u5)) & 0xFF) as u8;
            w += 1;
        }
        i += 1;
    }

    return w as i64;
}

/// Returns true if every character in `input` belongs to the z-base-32 alphabet (case-insensitive).
pub fn isValidZBase32(input: string) -> bool {
    let i: usize = 0;
    const len = input.len;
    const v_a: Zb32Vec16 = @splat(97 as u8);
    const v_z: Zb32Vec16 = @splat(122 as u8);
    const v_l: Zb32Vec16 = @splat(108 as u8);
    const v_v: Zb32Vec16 = @splat(118 as u8);
    const v_1: Zb32Vec16 = @splat(49 as u8);
    const v_3: Zb32Vec16 = @splat(51 as u8);
    const v_9: Zb32Vec16 = @splat(57 as u8);
    const v_case: Zb32Vec16 = @splat(32 as u8);
    while (i + 16 <= len) {
        const v: Zb32Vec16 = input[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const is_alpha = (vl >= v_a) & (vl <= v_z) & (vl != v_l) & (vl != v_v);
        const is_digit = (v == v_1) | ((v >= v_3) & (v <= v_9));
        if (!@reduce(.And, is_alpha | is_digit)) {
            return false;
        }
        i += 16;
    }
    while (i < len) {
        if (ZB32_DEC_LUT[input[i] as usize] >= 32) {
            return false;
        }
        i += 1;
    }
    return true;
}
