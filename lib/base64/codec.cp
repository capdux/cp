// Source: bun/src/base64/base64.cp -> cp/lib/base64/codec.cp (Zero-alloc & allocator Base64 codecs)

const std = @import("std");
import {
    Base64Variant,
    Base64Error,
    PAD_CHAR,
    INVALID_INDEX,
    hasPadding,
    alphabetFor,
    decodeTableFor,
    isWhitespace,
} from "./types.cp";

/// Returns the exact number of bytes required to encode `input_len` bytes using `variant`.
pub fn encodedLen(input_len: usize, variant: Base64Variant): usize {
    if (hasPadding(variant)) {
        return ((input_len + 2) / 3) * 4;
    }
    let full_groups = (input_len / 3) * 4;
    let rem = input_len % 3;
    if (rem == 0) {
        return full_groups;
    }
    return full_groups + rem + 1;
}

/// Returns an upper bound on the number of decoded bytes for an input of `encoded_len` bytes.
pub fn maxDecodedLen(encoded_len: usize): usize {
    let full = (encoded_len / 4) * 3;
    let rem = encoded_len % 4;
    if (rem == 2) {
        return full + 1;
    }
    if (rem == 3) {
        return full + 2;
    }
    return full;
}

/// Computes the exact decoded byte length of `encoded` under `variant`, validating length and padding.
pub fn exactDecodedLen(encoded: []const u8, variant: Base64Variant): Base64Error!usize {
    let n = encoded.len;
    if (n == 0) {
        return 0;
    }
    if (n % 4 == 1) {
        return err.InvalidLength;
    }
    if (hasPadding(variant)) {
        if (n % 4 != 0) {
            return err.InvalidLength;
        }
        let result = (n / 4) * 3;
        if (encoded[n - 1] == PAD_CHAR) {
            result -= 1;
            if (encoded[n - 2] == PAD_CHAR) {
                result -= 1;
            }
        }
        return result;
    }
    if (encoded[n - 1] == PAD_CHAR) {
        return err.InvalidPadding;
    }
    return maxDecodedLen(n);
}

/// Encodes `src` into `dst` using `variant`, returning the written slice.
pub fn encodeToSlice(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let out_len = encodedLen(src.len, variant);
    if (dst.len < out_len) {
        return err.DestinationTooSmall;
    }
    if (src.len == 0) {
        return dst[0..0];
    }

    let alphabet = alphabetFor(variant);
    let pad = hasPadding(variant);
    let in_idx: usize = 0;
    let out_idx: usize = 0;

    while (in_idx + 3 <= src.len) {
        let b0 = src[in_idx] as u32;
        let b1 = src[in_idx + 1] as u32;
        let b2 = src[in_idx + 2] as u32;
        let triple = (b0 << 16) | (b1 << 8) | b2;

        dst[out_idx] = alphabet[((triple >> 18) & 0x3F) as usize];
        dst[out_idx + 1] = alphabet[((triple >> 12) & 0x3F) as usize];
        dst[out_idx + 2] = alphabet[((triple >> 6) & 0x3F) as usize];
        dst[out_idx + 3] = alphabet[(triple & 0x3F) as usize];

        in_idx += 3;
        out_idx += 4;
    }

    let rem = src.len - in_idx;
    if (rem == 1) {
        let b0 = src[in_idx] as u32;
        dst[out_idx] = alphabet[((b0 >> 2) & 0x3F) as usize];
        dst[out_idx + 1] = alphabet[((b0 << 4) & 0x3F) as usize];
        out_idx += 2;
        if (pad) {
            dst[out_idx] = PAD_CHAR;
            dst[out_idx + 1] = PAD_CHAR;
            out_idx += 2;
        }
    } else if (rem == 2) {
        let b0 = src[in_idx] as u32;
        let b1 = src[in_idx + 1] as u32;
        let pair = (b0 << 8) | b1;
        dst[out_idx] = alphabet[((pair >> 10) & 0x3F) as usize];
        dst[out_idx + 1] = alphabet[((pair >> 4) & 0x3F) as usize];
        dst[out_idx + 2] = alphabet[((pair << 2) & 0x3F) as usize];
        out_idx += 3;
        if (pad) {
            dst[out_idx] = PAD_CHAR;
            out_idx += 1;
        }
    }

    return dst[0..out_idx];
}

/// Decodes a 2-, 3-, or 4-character tail quantum and enforces RFC 4648 §3.5 canonical zero padding bits.
pub fn decodeTailQuantum(
    dst: []u8,
    out_idx: usize,
    q0: u8,
    q1: u8,
    q2: u8,
    has_q2: bool,
    q3: u8,
    has_q3: bool,
): Base64Error!usize {
    if (!has_q2) {
        // 2 base64 chars -> 1 byte, lower 4 bits of q1 must be 0
        if ((q1 & 0x0F) != 0) {
            return err.NonCanonicalPaddingBits;
        }
        if (out_idx + 1 > dst.len) {
            return err.DestinationTooSmall;
        }
        dst[out_idx] = (((q0 as u32) << 2) | ((q1 as u32) >> 4)) as u8;
        return out_idx + 1;
    }
    if (!has_q3) {
        // 3 base64 chars -> 2 bytes, lower 2 bits of q2 must be 0
        if ((q2 & 0x03) != 0) {
            return err.NonCanonicalPaddingBits;
        }
        if (out_idx + 2 > dst.len) {
            return err.DestinationTooSmall;
        }
        let val = ((q0 as u32) << 12) | ((q1 as u32) << 6) | (q2 as u32);
        dst[out_idx] = ((val >> 10) & 0xFF) as u8;
        dst[out_idx + 1] = ((val >> 2) & 0xFF) as u8;
        return out_idx + 2;
    }
    if (out_idx + 3 > dst.len) {
        return err.DestinationTooSmall;
    }
    let val = ((q0 as u32) << 18) | ((q1 as u32) << 12) | ((q2 as u32) << 6) | (q3 as u32);
    dst[out_idx] = ((val >> 16) & 0xFF) as u8;
    dst[out_idx + 1] = ((val >> 8) & 0xFF) as u8;
    dst[out_idx + 2] = (val & 0xFF) as u8;
    return out_idx + 3;
}

/// Decodes `src` into `dst` using `variant` with strict RFC 4648 §3.5 validation.
pub fn decodeToSlice(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let n = src.len;
    if (n == 0) {
        return dst[0..0];
    }
    if (n % 4 == 1) {
        return err.InvalidLength;
    }

    let pad = hasPadding(variant);
    if (pad && n % 4 != 0) {
        return err.InvalidLength;
    }

    let table = decodeTableFor(variant);
    let out_idx: usize = 0;
    let in_idx: usize = 0;

    // Fast path for complete 4-char non-padded quanta before the final quantum.
    let limit = if (n >= 4) { n - 4 } else { 0 };
    while (in_idx < limit) {
        let c0 = src[in_idx];
        let c1 = src[in_idx + 1];
        let c2 = src[in_idx + 2];
        let c3 = src[in_idx + 3];

        let d0 = table[c0 as usize];
        let d1 = table[c1 as usize];
        let d2 = table[c2 as usize];
        let d3 = table[c3 as usize];

        if (d0 == INVALID_INDEX || d1 == INVALID_INDEX || d2 == INVALID_INDEX || d3 == INVALID_INDEX) {
            if (c0 == PAD_CHAR || c1 == PAD_CHAR || c2 == PAD_CHAR || c3 == PAD_CHAR) {
                return err.InvalidPadding;
            }
            return err.InvalidCharacter;
        }
        if (out_idx + 3 > dst.len) {
            return err.DestinationTooSmall;
        }

        let val = ((d0 as u32) << 18) | ((d1 as u32) << 12) | ((d2 as u32) << 6) | (d3 as u32);
        dst[out_idx] = ((val >> 16) & 0xFF) as u8;
        dst[out_idx + 1] = ((val >> 8) & 0xFF) as u8;
        dst[out_idx + 2] = (val & 0xFF) as u8;

        in_idx += 4;
        out_idx += 3;
    }

    // Handle final 2, 3, or 4 characters.
    let rem = n - in_idx;
    if (rem == 2) {
        let c0 = src[in_idx];
        let c1 = src[in_idx + 1];
        if (c0 == PAD_CHAR || c1 == PAD_CHAR) {
            return err.InvalidPadding;
        }
        let d0 = table[c0 as usize];
        let d1 = table[c1 as usize];
        if (d0 == INVALID_INDEX || d1 == INVALID_INDEX) {
            return err.InvalidCharacter;
        }
        out_idx = decodeTailQuantum(dst, out_idx, d0, d1, 0, false, 0, false)?;
        return dst[0..out_idx];
    }
    if (rem == 3) {
        let c0 = src[in_idx];
        let c1 = src[in_idx + 1];
        let c2 = src[in_idx + 2];
        if (c0 == PAD_CHAR || c1 == PAD_CHAR || c2 == PAD_CHAR) {
            return err.InvalidPadding;
        }
        let d0 = table[c0 as usize];
        let d1 = table[c1 as usize];
        let d2 = table[c2 as usize];
        if (d0 == INVALID_INDEX || d1 == INVALID_INDEX || d2 == INVALID_INDEX) {
            return err.InvalidCharacter;
        }
        out_idx = decodeTailQuantum(dst, out_idx, d0, d1, d2, true, 0, false)?;
        return dst[0..out_idx];
    }

    // rem == 4
    let c0 = src[in_idx];
    let c1 = src[in_idx + 1];
    let c2 = src[in_idx + 2];
    let c3 = src[in_idx + 3];

    if (c0 == PAD_CHAR || c1 == PAD_CHAR) {
        return err.InvalidPadding;
    }
    let d0 = table[c0 as usize];
    let d1 = table[c1 as usize];
    if (d0 == INVALID_INDEX || d1 == INVALID_INDEX) {
        return err.InvalidCharacter;
    }

    if (c2 == PAD_CHAR) {
        if (!pad) {
            return err.InvalidPadding;
        }
        if (c3 != PAD_CHAR) {
            return err.InvalidPadding;
        }
        out_idx = decodeTailQuantum(dst, out_idx, d0, d1, 0, false, 0, false)?;
        return dst[0..out_idx];
    }

    let d2 = table[c2 as usize];
    if (d2 == INVALID_INDEX) {
        return err.InvalidCharacter;
    }

    if (c3 == PAD_CHAR) {
        if (!pad) {
            return err.InvalidPadding;
        }
        out_idx = decodeTailQuantum(dst, out_idx, d0, d1, d2, true, 0, false)?;
        return dst[0..out_idx];
    }

    let d3 = table[c3 as usize];
    if (d3 == INVALID_INDEX) {
        return err.InvalidCharacter;
    }

    out_idx = decodeTailQuantum(dst, out_idx, d0, d1, d2, true, d3, true)?;
    return dst[0..out_idx];
}

/// Decodes `src` into `dst` using `variant`, ignoring ASCII whitespace (` `, `\r`, `\n`, `\t`).
pub fn decodeIgnoreWhitespaceToSlice(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let table = decodeTableFor(variant);
    let pad = hasPadding(variant);
    let q: [4]u8 = [0; 4];
    let q_len: usize = 0;
    let out_idx: usize = 0;
    let pad_count: usize = 0;
    let saw_pad = false;

    for (c in src) {
        if (isWhitespace(c)) {
            continue;
        }
        if (c == PAD_CHAR) {
            if (!pad) {
                return err.InvalidPadding;
            }
            if (!saw_pad) {
                if (q_len < 2) {
                    return err.InvalidPadding;
                }
                saw_pad = true;
            }
            pad_count += 1;
            if (q_len + pad_count > 4) {
                return err.InvalidPadding;
            }
            continue;
        }
        if (saw_pad) {
            return err.InvalidPadding;
        }
        let d = table[c as usize];
        if (d == INVALID_INDEX) {
            return err.InvalidCharacter;
        }
        q[q_len] = d;
        q_len += 1;
        if (q_len == 4) {
            out_idx = decodeTailQuantum(dst, out_idx, q[0], q[1], q[2], true, q[3], true)?;
            q_len = 0;
        }
    }

    if (saw_pad) {
        if (q_len + pad_count != 4) {
            return err.InvalidPadding;
        }
        if (q_len == 2) {
            out_idx = decodeTailQuantum(dst, out_idx, q[0], q[1], 0, false, 0, false)?;
            return dst[0..out_idx];
        }
        if (q_len == 3) {
            out_idx = decodeTailQuantum(dst, out_idx, q[0], q[1], q[2], true, 0, false)?;
            return dst[0..out_idx];
        }
        return err.InvalidPadding;
    }

    if (q_len == 0) {
        return dst[0..out_idx];
    }
    if (q_len == 1) {
        return err.InvalidLength;
    }
    if (pad) {
        return err.InvalidLength;
    }
    if (q_len == 2) {
        out_idx = decodeTailQuantum(dst, out_idx, q[0], q[1], 0, false, 0, false)?;
    } else {
        out_idx = decodeTailQuantum(dst, out_idx, q[0], q[1], q[2], true, 0, false)?;
    }
    return dst[0..out_idx];
}

/// Allocates and returns the Base64-encoded representation of `src` using `variant`.
pub fn encodeAlloc(alloc: std.mem.Allocator, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let out_len = encodedLen(src.len, variant);
    let buf = alloc.alloc(u8, out_len) catch return err.OutOfMemory;
    errdefer alloc.free(buf);
    return encodeToSlice(buf, src, variant);
}

/// Allocates and returns the Base64-decoded bytes of `src` using `variant`.
pub fn decodeAlloc(alloc: std.mem.Allocator, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let needed = exactDecodedLen(src, variant)?;
    let buf = alloc.alloc(u8, needed) catch return err.OutOfMemory;
    errdefer alloc.free(buf);
    return decodeToSlice(buf, src, variant);
}
