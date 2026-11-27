// @cap/base32-rfc — Zero-allocation RFC 4648 §6/§7 Standard Base32 & Base32hex Codec
// Clean-room MIT implementation in pure CP.

import { toUpperByte } from "cap:ascii";

const STD_ALPHABET: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const HEX_ALPHABET: string = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

fn decodeStdChar(b: u8) -> i32 {
    const u = toUpperByte(b);
    if (u >= 65 && u <= 90) {
        return (u - 65) as i32; // A-Z -> 0..25
    }
    if (u >= 50 && u <= 55) {
        return (u - 50 + 26) as i32; // 2-7 -> 26..31
    }
    return -1;
}

fn decodeHexChar(b: u8) -> i32 {
    const u = toUpperByte(b);
    if (u >= 48 && u <= 57) {
        return (u - 48) as i32; // 0-9 -> 0..9
    }
    if (u >= 65 && u <= 86) {
        return (u - 65 + 10) as i32; // A-V -> 10..31
    }
    return -1;
}

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

fn encodeWithAlphabet(data: []const u8, pad: bool, alphabet: string, out: []u8) -> usize {
    if (data.len == 0) {
        return 0;
    }
    const needed = encodedLen(data.len, pad);
    if (needed > out.len) {
        return 0;
    }

    let acc: u32 = 0;
    let bits: u32 = 0;
    let w: usize = 0;
    let i: usize = 0;

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

    let acc: u32 = 0;
    let bits: u32 = 0;
    let w: usize = 0;
    let i: usize = 0;

    while (i < data_len) {
        let val: i32 = -1;
        if (is_hex) {
            val = decodeHexChar(input[i]);
        } else {
            val = decodeStdChar(input[i]);
        }
        if (val < 0) {
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
    return encodeWithAlphabet(data, pad, STD_ALPHABET, out);
}

/// Decodes RFC 4648 Section 6 standard Base32 (`A-Z2-7`, case-insensitive, padded or unpadded).
/// Returns bytes written, or `-1` on invalid character, bad padding, or insufficient buffer.
pub fn decode(input: string, out: []u8) -> i64 {
    return decodeWithMode(input, false, out);
}

/// Encodes `data` using RFC 4648 Section 7 Extended Hex Base32 (`0-9A-V`).
/// Pads with `=` to a multiple of 8 bytes when `pad == true`. Returns bytes written, or `0` on overflow.
pub fn encodeHex(data: []const u8, pad: bool, out: []u8) -> usize {
    return encodeWithAlphabet(data, pad, HEX_ALPHABET, out);
}

/// Decodes RFC 4648 Section 7 Extended Hex Base32 (`0-9A-V`, case-insensitive, padded or unpadded).
/// Returns bytes written, or `-1` on invalid character, bad padding, or insufficient buffer.
pub fn decodeHex(input: string, out: []u8) -> i64 {
    return decodeWithMode(input, true, out);
}
