// @cap/hex-crate — Zero-heap hexadecimal encoding/decoding modeled on Rust's `hex` crate (1.5B+ downloads).
// Clean-room MIT implementation; reuses `@cap/data-encoding` with zero codec duplication.

import {
    hexLowerEncode, hexUpperEncode, hexDecode, DecodeResult
} from "../../data-encoding/src/index.cp";

/// Encodes `src` bytes as lowercase hexadecimal ASCII into `out`.
pub fn encodeHex(src: []const u8, out: []u8): usize {
    return hexLowerEncode(src, out);
}

/// Encodes `src` bytes as uppercase hexadecimal ASCII into `out`.
pub fn encodeHexUpper(src: []const u8, out: []u8): usize {
    return hexUpperEncode(src, out);
}

/// Decodes hexadecimal ASCII (lowercase or uppercase) into `out`, returning `DecodeResult`.
pub fn decodeHex(src: []const u8, out: []u8): DecodeResult {
    return hexDecode(src, out);
}

/// Returns exact hex-encoded byte length (`input_len * 2`).
pub fn hexEncodedLen(input_len: usize): usize {
    return input_len * 2;
}

/// Returns decoded byte length for a valid even-length hex string (`hex_len / 2`), or `null` if odd.
pub fn hexDecodedLen(hex_len: usize): usize? {
    if ((hex_len & 1) != 0) { return null; }
    return hex_len >> 1;
}

/// Validates whether `s` is an even-length valid hexadecimal ASCII string.
pub fn isValidHex(s: string): bool {
    if ((s.len & 1) != 0) { return false; }
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        let ok = (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
        if (!ok) { return false; }
        i = i + 1;
    }
    return true;
}

/// Strips optional `0x` / `0X` prefix before decoding hex into `out`.
pub fn decodeHexPrefixed(src: string, out: []u8): DecodeResult {
    let body = src;
    if (body.len >= 2 && body[0] == 48 && (body[1] == 120 || body[1] == 88)) {
        body = body[2..body.len];
    }
    return hexDecode(body, out);
}
