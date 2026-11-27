// @cap/hex — Zero-allocation hexadecimal encoder/decoder & canonical HexDump for CP
// Clean-room MIT implementation inspired by Rust `hex`.
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD validation,
// @memcpy fast paths, and uninitialized stack line buffers.

import { writeStr } from "cap:fmt";

type HexVec16 = @Vector(16, u8);

const HEX_LOWER_DIGITS: string = "0123456789abcdef";
const HEX_UPPER_DIGITS: string = "0123456789ABCDEF";

fn hexBuildDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = (c - 48) as u8;
        c += 1;
    }
    c = 65;
    while (c <= 70) {
        lut[c] = (c - 55) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 102) {
        lut[c] = (c - 87) as u8;
        c += 1;
    }
    return lut;
}

fn hexBuildPrintLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = ['.' as u8; 256];
    let c: usize = 32;
    while (c <= 126) {
        lut[c] = c as u8;
        c += 1;
    }
    return lut;
}

const HEX_DEC_LUT: [256]u8 = hexBuildDecLut();
const HEX_PRINT_LUT: [256]u8 = hexBuildPrintLut();

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

// ---------------------------------------------------------------------------
// 1. Hex Encoding & Decoding
// ---------------------------------------------------------------------------

/// Encodes `input` bytes as lowercase hexadecimal into `out`.
pub fn encodeHex(input: string, out: []u8) -> string {
    const max_in = out.len / 2;
    const n = if (input.len < max_in) input.len else max_in;
    let pos: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) {
        const b0 = input[i];
        const b1 = input[i + 1];
        const b2 = input[i + 2];
        const b3 = input[i + 3];
        out[pos] = HEX_LOWER_DIGITS[(b0 >> 4) as usize];
        out[pos + 1] = HEX_LOWER_DIGITS[(b0 & 0x0F) as usize];
        out[pos + 2] = HEX_LOWER_DIGITS[(b1 >> 4) as usize];
        out[pos + 3] = HEX_LOWER_DIGITS[(b1 & 0x0F) as usize];
        out[pos + 4] = HEX_LOWER_DIGITS[(b2 >> 4) as usize];
        out[pos + 5] = HEX_LOWER_DIGITS[(b2 & 0x0F) as usize];
        out[pos + 6] = HEX_LOWER_DIGITS[(b3 >> 4) as usize];
        out[pos + 7] = HEX_LOWER_DIGITS[(b3 & 0x0F) as usize];
        pos += 8;
        i += 4;
    }
    while (i < n) {
        const b = input[i];
        out[pos] = HEX_LOWER_DIGITS[(b >> 4) as usize];
        out[pos + 1] = HEX_LOWER_DIGITS[(b & 0x0F) as usize];
        pos += 2;
        i += 1;
    }
    return out[0..pos];
}

/// Encodes `input` bytes as uppercase hexadecimal into `out`.
pub fn encodeHexUpper(input: string, out: []u8) -> string {
    const max_in = out.len / 2;
    const n = if (input.len < max_in) input.len else max_in;
    let pos: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) {
        const b0 = input[i];
        const b1 = input[i + 1];
        const b2 = input[i + 2];
        const b3 = input[i + 3];
        out[pos] = HEX_UPPER_DIGITS[(b0 >> 4) as usize];
        out[pos + 1] = HEX_UPPER_DIGITS[(b0 & 0x0F) as usize];
        out[pos + 2] = HEX_UPPER_DIGITS[(b1 >> 4) as usize];
        out[pos + 3] = HEX_UPPER_DIGITS[(b1 & 0x0F) as usize];
        out[pos + 4] = HEX_UPPER_DIGITS[(b2 >> 4) as usize];
        out[pos + 5] = HEX_UPPER_DIGITS[(b2 & 0x0F) as usize];
        out[pos + 6] = HEX_UPPER_DIGITS[(b3 >> 4) as usize];
        out[pos + 7] = HEX_UPPER_DIGITS[(b3 & 0x0F) as usize];
        pos += 8;
        i += 4;
    }
    while (i < n) {
        const b = input[i];
        out[pos] = HEX_UPPER_DIGITS[(b >> 4) as usize];
        out[pos + 1] = HEX_UPPER_DIGITS[(b & 0x0F) as usize];
        pos += 2;
        i += 1;
    }
    return out[0..pos];
}

/// Returns `true` if `s` has even length and consists solely of ASCII hex digits `[0-9a-fA-F]`.
pub fn isValidHex(s: string) -> bool {
    if ((s.len & 1) != 0) {
        return false;
    }
    let i: usize = 0;
    const len = s.len;
    const v_0: HexVec16 = @splat(48 as u8);
    const v_9: HexVec16 = @splat(57 as u8);
    const v_a: HexVec16 = @splat(97 as u8);
    const v_f: HexVec16 = @splat(102 as u8);
    const v_case: HexVec16 = @splat(32 as u8);
    while (i + 16 <= len) {
        const v: HexVec16 = s[i .. i + 16][0..16].*;
        const vl = v | v_case;
        const is_digit = (v >= v_0) & (v <= v_9);
        const is_alpha = (vl >= v_a) & (vl <= v_f);
        if (!@reduce(.And, is_digit | is_alpha)) {
            return false;
        }
        i += 16;
    }
    while (i < len) {
        if (HEX_DEC_LUT[s[i] as usize] == 255) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Decodes a hexadecimal string into `out`.
/// Returns `null` if length is odd, any byte is not a valid hex digit, or `out` is too small.
pub fn decodeHex(hex_str: string, out: []u8) -> string? {
    if ((hex_str.len & 1) != 0) {
        return null;
    }
    const out_len = hex_str.len >> 1;
    if (out.len < out_len) {
        return null;
    }
    let i: usize = 0;
    while (i + 4 <= out_len) {
        const p = i << 1;
        const h0 = HEX_DEC_LUT[hex_str[p] as usize];
        const l0 = HEX_DEC_LUT[hex_str[p + 1] as usize];
        const h1 = HEX_DEC_LUT[hex_str[p + 2] as usize];
        const l1 = HEX_DEC_LUT[hex_str[p + 3] as usize];
        const h2 = HEX_DEC_LUT[hex_str[p + 4] as usize];
        const l2 = HEX_DEC_LUT[hex_str[p + 5] as usize];
        const h3 = HEX_DEC_LUT[hex_str[p + 6] as usize];
        const l3 = HEX_DEC_LUT[hex_str[p + 7] as usize];
        if (((h0 | l0 | h1 | l1 | h2 | l2 | h3 | l3) & 0xF0) != 0) {
            return null;
        }
        out[i] = (h0 << 4) | l0;
        out[i + 1] = (h1 << 4) | l1;
        out[i + 2] = (h2 << 4) | l2;
        out[i + 3] = (h3 << 4) | l3;
        i += 4;
    }
    while (i < out_len) {
        const p = i << 1;
        const hi = HEX_DEC_LUT[hex_str[p] as usize];
        const lo = HEX_DEC_LUT[hex_str[p + 1] as usize];
        if (((hi | lo) & 0xF0) != 0) {
            return null;
        }
        out[i] = (hi << 4) | lo;
        i += 1;
    }
    return out[0..out_len];
}

/// Decodes a hexadecimal string with an optional `0x` / `0X` prefix into `out`.
pub fn decodeHexPrefixed(hex_str: string, out: []u8) -> string? {
    let s = hex_str;
    if (s.len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s = s[2..s.len];
    }
    return decodeHex(s, out);
}

// ---------------------------------------------------------------------------
// 2. Fixed-Width Integer Hex Formatters
// ---------------------------------------------------------------------------

/// Encodes `v` as a fixed 8-character lowercase big-endian hexadecimal string into `out`.
pub fn encodeU32Hex(v: u32, out: []u8) -> string {
    if (out.len < 8) {
        return out[0..0];
    }
    out[0] = HEX_LOWER_DIGITS[((v >> 28) & 0x0F) as usize];
    out[1] = HEX_LOWER_DIGITS[((v >> 24) & 0x0F) as usize];
    out[2] = HEX_LOWER_DIGITS[((v >> 20) & 0x0F) as usize];
    out[3] = HEX_LOWER_DIGITS[((v >> 16) & 0x0F) as usize];
    out[4] = HEX_LOWER_DIGITS[((v >> 12) & 0x0F) as usize];
    out[5] = HEX_LOWER_DIGITS[((v >> 8) & 0x0F) as usize];
    out[6] = HEX_LOWER_DIGITS[((v >> 4) & 0x0F) as usize];
    out[7] = HEX_LOWER_DIGITS[(v & 0x0F) as usize];
    return out[0..8];
}

/// Encodes `v` as a fixed 16-character lowercase big-endian hexadecimal string into `out`.
pub fn encodeU64Hex(v: u64, out: []u8) -> string {
    if (out.len < 16) {
        return out[0..0];
    }
    let i: usize = 0;
    while (i < 16) {
        const shift: u64 = ((15 - i) * 4) as u64;
        out[i] = HEX_LOWER_DIGITS[((v >> shift) & 0x0F) as usize];
        i += 1;
    }
    return out[0..16];
}

// ---------------------------------------------------------------------------
// 3. Canonical 16-Byte/Line HexDump Formatter
// ---------------------------------------------------------------------------

/// Formats `data` as a canonical 16-bytes-per-line Hex+ASCII dump into `out`:
/// `00000000  48 65 6c 6c 6f ...  |Hello...|\n`
pub fn hexDump(data: string, out: []u8) -> string {
    if (data.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let offset: usize = 0;
    let line_buf: [80]u8 = undefined;
    while (offset < data.len && pos < out.len) {
        let lp: usize = 0;
        const off_u32 = offset as u32;
        line_buf[0] = HEX_LOWER_DIGITS[((off_u32 >> 28) & 0x0F) as usize];
        line_buf[1] = HEX_LOWER_DIGITS[((off_u32 >> 24) & 0x0F) as usize];
        line_buf[2] = HEX_LOWER_DIGITS[((off_u32 >> 20) & 0x0F) as usize];
        line_buf[3] = HEX_LOWER_DIGITS[((off_u32 >> 16) & 0x0F) as usize];
        line_buf[4] = HEX_LOWER_DIGITS[((off_u32 >> 12) & 0x0F) as usize];
        line_buf[5] = HEX_LOWER_DIGITS[((off_u32 >> 8) & 0x0F) as usize];
        line_buf[6] = HEX_LOWER_DIGITS[((off_u32 >> 4) & 0x0F) as usize];
        line_buf[7] = HEX_LOWER_DIGITS[(off_u32 & 0x0F) as usize];
        line_buf[8] = ' ' as u8;
        line_buf[9] = ' ' as u8;
        lp = 10;

        let col: usize = 0;
        while (col < 16) {
            if (col == 8) {
                line_buf[lp] = ' ' as u8;
                lp += 1;
            }
            if (offset + col < data.len) {
                const b = data[offset + col];
                line_buf[lp] = HEX_LOWER_DIGITS[(b >> 4) as usize];
                line_buf[lp + 1] = HEX_LOWER_DIGITS[(b & 0x0F) as usize];
                line_buf[lp + 2] = ' ' as u8;
            } else {
                line_buf[lp] = ' ' as u8;
                line_buf[lp + 1] = ' ' as u8;
                line_buf[lp + 2] = ' ' as u8;
            }
            lp += 3;
            col += 1;
        }

        line_buf[lp] = ' ' as u8;
        line_buf[lp + 1] = '|' as u8;
        lp += 2;

        col = 0;
        while (col < 16 && offset + col < data.len) {
            line_buf[lp] = HEX_PRINT_LUT[data[offset + col] as usize];
            lp += 1;
            col += 1;
        }
        line_buf[lp] = '|' as u8;
        line_buf[lp + 1] = '\n' as u8;
        lp += 2;

        const avail = out.len - pos;
        const copy_len = if (lp < avail) lp else avail;
        @memcpy(out[pos .. pos + copy_len], line_buf[0..copy_len]);
        pos += copy_len;

        offset += 16;
    }
    return out[0..pos];
}
