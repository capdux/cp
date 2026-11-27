// @cap/zbase32 — Zero-allocation Zooko Human-Oriented z-base-32 Codec
// Clean-room MIT implementation in pure CP.
// Alphabet: "ybndrfg8ejkmcpqxot1uwisza345h769" (unpadded, big-endian 5-bit groups).

import { toLowerByte } from "cap:ascii";

const ALPHABET: string = "ybndrfg8ejkmcpqxot1uwisza345h769";

fn decodeChar(b: u8) -> i32 {
    const c = toLowerByte(b);
    if (c == 121) { return 0; }  // y
    if (c == 98) { return 1; }   // b
    if (c == 110) { return 2; }  // n
    if (c == 100) { return 3; }  // d
    if (c == 114) { return 4; }  // r
    if (c == 102) { return 5; }  // f
    if (c == 103) { return 6; }  // g
    if (c == 56) { return 7; }   // 8
    if (c == 101) { return 8; }  // e
    if (c == 106) { return 9; }  // j
    if (c == 107) { return 10; } // k
    if (c == 109) { return 11; } // m
    if (c == 99) { return 12; }  // c
    if (c == 112) { return 13; } // p
    if (c == 113) { return 14; } // q
    if (c == 120) { return 15; } // x
    if (c == 111) { return 16; } // o
    if (c == 116) { return 17; } // t
    if (c == 49) { return 18; }  // 1
    if (c == 117) { return 19; } // u
    if (c == 119) { return 20; } // w
    if (c == 105) { return 21; } // i
    if (c == 115) { return 22; } // s
    if (c == 122) { return 23; } // z
    if (c == 97) { return 24; }  // a
    if (c == 51) { return 25; }  // 3
    if (c == 52) { return 26; }  // 4
    if (c == 53) { return 27; }  // 5
    if (c == 104) { return 28; } // h
    if (c == 55) { return 29; }  // 7
    if (c == 54) { return 30; }  // 6
    if (c == 57) { return 31; }  // 9
    return -1;
}

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

    let acc: u32 = 0;
    let bits: u32 = 0;
    let w: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        const val = decodeChar(input[i]);
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

/// Returns true if every character in `input` belongs to the z-base-32 alphabet (case-insensitive).
pub fn isValidZBase32(input: string) -> bool {
    let i: usize = 0;
    while (i < input.len) {
        if (decodeChar(input[i]) < 0) {
            return false;
        }
        i += 1;
    }
    return true;
}
