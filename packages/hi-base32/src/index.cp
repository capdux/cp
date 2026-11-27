// @cap/base32-rfc — Zero-allocation RFC 4648 §6/§7 Standard Base32 & Base32hex Codec
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD encode/decode validation, comptime 256-entry decode LUTs, @memcpy, and undefined stack buffers.

const STD_ALPHABET: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const HEX_ALPHABET: string = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

type B32Vec16 = @Vector(16, u8);

fn b32BuildStdDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = 0xFF;
        i += 1;
    }
    let c: usize = 0;
    while (c < 26) {
        lut[65 + c] = c as u8; // A-Z
        lut[97 + c] = c as u8; // a-z
        c += 1;
    }
    let d: usize = 0;
    while (d < 6) {
        lut[50 + d] = (26 + d) as u8; // 2-7
        d += 1;
    }
    return lut;
}

fn b32BuildHexDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = 0xFF;
        i += 1;
    }
    let d: usize = 0;
    while (d < 10) {
        lut[48 + d] = d as u8; // 0-9
        d += 1;
    }
    let c: usize = 0;
    while (c < 22) {
        lut[65 + c] = (10 + c) as u8; // A-V
        lut[97 + c] = (10 + c) as u8; // a-v
        c += 1;
    }
    return lut;
}

const B32_STD_DEC_LUT: [256]u8 = b32BuildStdDecLut();
const B32_HEX_DEC_LUT: [256]u8 = b32BuildHexDecLut();

/// Returns the exact Base32 encoded length for `byte_len` bytes, with or without `=` padding.
pub fn encodedLen(byte_len: usize, pad: bool) -> usize {
    if (byte_len == 0) {
        return 0;
    }
    if (pad) {
        return ((byte_len + 4) / 5) * 8;
    }
    return (byte_len * 8 + 4) / 5;
}

fn encodeWithMode(data: []const u8, pad: bool, is_hex: bool, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    const needed = encodedLen(data.len, pad);
    if (needed > out.len) {
        return 0;
    }

    const alphabet: string = if (is_hex) HEX_ALPHABET else STD_ALPHABET;
    let w: usize = 0;
    let i: usize = 0;

    // 16-lane SIMD fast path: 10 input bytes -> 16 Base32 characters per iteration
    const v_thresh: B32Vec16 = if (is_hex) @splat(10 as u8) else @splat(26 as u8);
    const v_lo_off: B32Vec16 = if (is_hex) @splat(48 as u8) else @splat(65 as u8);
    const v_hi_off: B32Vec16 = if (is_hex) @splat(55 as u8) else @splat(24 as u8);

    while (i + 10 <= data.len) {
        let idx16: [16]u8 = undefined;
        const a0 = data[i];
        const a1 = data[i + 1];
        const a2 = data[i + 2];
        const a3 = data[i + 3];
        const a4 = data[i + 4];
        idx16[0] = a0 >> 3;
        idx16[1] = ((a0 & 0x07) << 2) | (a1 >> 6);
        idx16[2] = (a1 >> 1) & 0x1F;
        idx16[3] = ((a1 & 0x01) << 4) | (a2 >> 4);
        idx16[4] = ((a2 & 0x0F) << 1) | (a3 >> 7);
        idx16[5] = (a3 >> 2) & 0x1F;
        idx16[6] = ((a3 & 0x03) << 3) | (a4 >> 5);
        idx16[7] = a4 & 0x1F;

        const b0 = data[i + 5];
        const b1 = data[i + 6];
        const b2 = data[i + 7];
        const b3 = data[i + 8];
        const b4 = data[i + 9];
        idx16[8] = b0 >> 3;
        idx16[9] = ((b0 & 0x07) << 2) | (b1 >> 6);
        idx16[10] = (b1 >> 1) & 0x1F;
        idx16[11] = ((b1 & 0x01) << 4) | (b2 >> 4);
        idx16[12] = ((b2 & 0x0F) << 1) | (b3 >> 7);
        idx16[13] = (b3 >> 2) & 0x1F;
        idx16[14] = ((b3 & 0x03) << 3) | (b4 >> 5);
        idx16[15] = b4 & 0x1F;

        const vi: B32Vec16 = idx16;
        const v_out: B32Vec16 = vi + @select(u8, vi < v_thresh, v_lo_off, v_hi_off);
        const out_arr: [16]u8 = v_out;
        @memcpy(out[w .. w + 16], out_arr[0..16]);
        i += 10;
        w += 16;
    }

    // 5-byte -> 8-char block
    if (i + 5 <= data.len) {
        const a0 = data[i];
        const a1 = data[i + 1];
        const a2 = data[i + 2];
        const a3 = data[i + 3];
        const a4 = data[i + 4];
        out[w] = alphabet[(a0 >> 3) as usize];
        out[w + 1] = alphabet[(((a0 & 0x07) << 2) | (a1 >> 6)) as usize];
        out[w + 2] = alphabet[((a1 >> 1) & 0x1F) as usize];
        out[w + 3] = alphabet[(((a1 & 0x01) << 4) | (a2 >> 4)) as usize];
        out[w + 4] = alphabet[(((a2 & 0x0F) << 1) | (a3 >> 7)) as usize];
        out[w + 5] = alphabet[((a3 >> 2) & 0x1F) as usize];
        out[w + 6] = alphabet[(((a3 & 0x03) << 3) | (a4 >> 5)) as usize];
        out[w + 7] = alphabet[(a4 & 0x1F) as usize];
        i += 5;
        w += 8;
    }

    // Trailing 1..4 bytes
    if (i < data.len) {
        let acc: u32 = 0;
        let bits: u32 = 0;
        while (i < data.len) {
            acc = (acc << 8) | (data[i] as u32);
            bits += 8;
            while (bits >= 5) {
                bits -= 5;
                const idx = ((acc >> (bits as u5)) & 0x1F) as usize;
                out[w] = alphabet[idx];
                w += 1;
            }
            i += 1;
        }
        if (bits > 0) {
            const idx = ((acc << ((5 - bits) as u5)) & 0x1F) as usize;
            out[w] = alphabet[idx];
            w += 1;
        }
    }

    while (w < needed) {
        out[w] = 61; // '='
        w += 1;
    }

    return w;
}

fn decodeWithMode(input: string, is_hex: bool, out: []u8) -> i64 {
    const n = input.len;
    if (n == 0) {
        return 0;
    }

    let data_len = n;
    while (data_len > 0 && input[data_len - 1] == 61) {
        data_len -= 1;
    }
    const pad_count = n - data_len;

    if (pad_count > 0) {
        if ((n % 8) != 0) {
            return -1;
        }
        if (pad_count != 1 && pad_count != 3 && pad_count != 4 && pad_count != 6) {
            return -1;
        }
    }

    const rem = data_len % 8;
    if (rem == 1 || rem == 3 || rem == 6) {
        return -1;
    }

    const out_needed = (data_len * 5) / 8;
    if (out_needed > out.len) {
        return -1;
    }

    const lut = if (is_hex) B32_HEX_DEC_LUT else B32_STD_DEC_LUT;
    const v_invalid: B32Vec16 = @splat(32 as u8);

    let w: usize = 0;
    let i: usize = 0;

    // 16-lane SIMD validation + 16 chars -> 10 bytes unrolled decode loop
    while (i + 16 <= data_len) {
        let vals: [16]u8 = undefined;
        vals[0] = lut[input[i] as usize];
        vals[1] = lut[input[i + 1] as usize];
        vals[2] = lut[input[i + 2] as usize];
        vals[3] = lut[input[i + 3] as usize];
        vals[4] = lut[input[i + 4] as usize];
        vals[5] = lut[input[i + 5] as usize];
        vals[6] = lut[input[i + 6] as usize];
        vals[7] = lut[input[i + 7] as usize];
        vals[8] = lut[input[i + 8] as usize];
        vals[9] = lut[input[i + 9] as usize];
        vals[10] = lut[input[i + 10] as usize];
        vals[11] = lut[input[i + 11] as usize];
        vals[12] = lut[input[i + 12] as usize];
        vals[13] = lut[input[i + 13] as usize];
        vals[14] = lut[input[i + 14] as usize];
        vals[15] = lut[input[i + 15] as usize];

        const vv: B32Vec16 = vals;
        if (@reduce(.Or, vv >= v_invalid)) {
            return -1;
        }

        out[w] = (vals[0] << 3) | (vals[1] >> 2);
        out[w + 1] = ((vals[1] & 0x03) << 6) | (vals[2] << 1) | (vals[3] >> 4);
        out[w + 2] = ((vals[3] & 0x0F) << 4) | (vals[4] >> 1);
        out[w + 3] = ((vals[4] & 0x01) << 7) | (vals[5] << 2) | (vals[6] >> 3);
        out[w + 4] = ((vals[6] & 0x07) << 5) | vals[7];

        out[w + 5] = (vals[8] << 3) | (vals[9] >> 2);
        out[w + 6] = ((vals[9] & 0x03) << 6) | (vals[10] << 1) | (vals[11] >> 4);
        out[w + 7] = ((vals[11] & 0x0F) << 4) | (vals[12] >> 1);
        out[w + 8] = ((vals[12] & 0x01) << 7) | (vals[13] << 2) | (vals[14] >> 3);
        out[w + 9] = ((vals[14] & 0x07) << 5) | vals[15];

        i += 16;
        w += 10;
    }

    // Scalar tail (< 16 chars)
    let acc: u32 = 0;
    let bits: u32 = 0;
    while (i < data_len) {
        const val = lut[input[i] as usize];
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

/// Encodes `data` using RFC 4648 Section 6 standard Base32 (`A-Z2-7`).
/// Pads with `=` to a multiple of 8 bytes when `pad == true`. Returns bytes written, or `0` on overflow.
pub fn encode(data: []const u8, pad: bool, out: []u8) -> usize {
    return encodeWithMode(data, pad, false, out);
}

/// Decodes RFC 4648 Section 6 standard Base32 (`A-Z2-7`, case-insensitive, padded or unpadded).
/// Returns bytes written, or `-1` on invalid character, bad padding, or insufficient buffer.
pub fn decode(input: string, out: []u8) -> i64 {
    return decodeWithMode(input, false, out);
}

/// Encodes `data` using RFC 4648 Section 7 Extended Hex Base32 (`0-9A-V`).
/// Pads with `=` to a multiple of 8 bytes when `pad == true`. Returns bytes written, or `0` on overflow.
pub fn encodeHex(data: []const u8, pad: bool, out: []u8) -> usize {
    return encodeWithMode(data, pad, true, out);
}

/// Decodes RFC 4648 Section 7 Extended Hex Base32 (`0-9A-V`, case-insensitive, padded or unpadded).
/// Returns bytes written, or `-1` on invalid character, bad padding, or insufficient buffer.
pub fn decodeHex(input: string, out: []u8) -> i64 {
    return decodeWithMode(input, true, out);
}
