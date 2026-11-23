// Reference-hardened against marshallpierce/rust-base64 (MIT/Apache-2.0)
// Source: bun/src/base64/base64.cp -> cp/lib/base64/codec.cp (Zero-alloc, 16-lane SIMD & allocator Base64 codecs)

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

type B64Vec16u8 = @Vector(16, u8);

fn isUrlSafeVariant(variant: Base64Variant): bool {
    switch (variant) {
        .UrlSafe, .UrlSafeNoPad => return true,
        .Standard, .StandardNoPad => return false,
    }
}

/// 16-lane SIMD alphabet preflight check for 16 bytes (Standard: `A-Za-z0-9+/`, URL-Safe: `A-Za-z0-9-_`).
pub fn simdValidateBlock16(block: []const u8, url_safe: bool): bool {
    if (block.len < 16) return false;
    let raw: [16]u8 = [
        block[0], block[1], block[2], block[3],
        block[4], block[5], block[6], block[7],
        block[8], block[9], block[10], block[11],
        block[12], block[13], block[14], block[15],
    ];
    const v: B64Vec16u8 = raw;
    const v_A: B64Vec16u8 = @splat(65 as u8);
    const v_Z: B64Vec16u8 = @splat(90 as u8);
    const v_a: B64Vec16u8 = @splat(97 as u8);
    const v_z: B64Vec16u8 = @splat(122 as u8);
    const v_0: B64Vec16u8 = @splat(48 as u8);
    const v_9: B64Vec16u8 = @splat(57 as u8);
    const c62: u8 = if (url_safe) (45 as u8) else (43 as u8); // '-' vs '+'
    const c63: u8 = if (url_safe) (95 as u8) else (47 as u8); // '_' vs '/'
    const v_62: B64Vec16u8 = @splat(c62);
    const v_63: B64Vec16u8 = @splat(c63);

    const m_upper: u16 = @as(u16, @bitCast(v >= v_A)) & @as(u16, @bitCast(v <= v_Z));
    const m_lower: u16 = @as(u16, @bitCast(v >= v_a)) & @as(u16, @bitCast(v <= v_z));
    const m_digit: u16 = @as(u16, @bitCast(v >= v_0)) & @as(u16, @bitCast(v <= v_9));
    const m_62: u16 = @bitCast(v == v_62);
    const m_63: u16 = @bitCast(v == v_63);
    return (m_upper | m_lower | m_digit | m_62 | m_63) == 0xFFFF;
}

/// Scalar reference for `simdValidateBlock16` (used for differential verification).
pub fn simdValidateBlock16Scalar(block: []const u8, url_safe: bool): bool {
    if (block.len < 16) return false;
    const c62: u8 = if (url_safe) (45 as u8) else (43 as u8);
    const c63: u8 = if (url_safe) (95 as u8) else (47 as u8);
    let i: usize = 0;
    while (i < 16) {
        const c = block[i];
        const ok = (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == c62 || c == c63;
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// 16-lane SIMD validator for an unpadded Base64 slice.
pub fn simdValidateBase64Alphabet(input: []const u8, url_safe: bool): bool {
    let i: usize = 0;
    while (i + 16 <= input.len) {
        if (!simdValidateBlock16(input[i .. i + 16], url_safe)) return false;
        i += 16;
    }
    const c62: u8 = if (url_safe) (45 as u8) else (43 as u8);
    const c63: u8 = if (url_safe) (95 as u8) else (47 as u8);
    while (i < input.len) {
        const c = input[i];
        const ok = (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == c62 || c == c63;
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// 16-lane SIMD encoder fast-path: converts 12 raw bytes into 16 Base64 ASCII bytes without lookup tables.
pub fn simdEncodeChunk12To16(src12: []const u8, dst16: []u8, url_safe: bool) {
    let idx: [16]u8 = [
        (src12[0] >> 2),
        ((src12[0] & 0x03) << 4) | (src12[1] >> 4),
        ((src12[1] & 0x0F) << 2) | (src12[2] >> 6),
        (src12[2] & 0x3F),
        (src12[3] >> 2),
        ((src12[3] & 0x03) << 4) | (src12[4] >> 4),
        ((src12[4] & 0x0F) << 2) | (src12[5] >> 6),
        (src12[5] & 0x3F),
        (src12[6] >> 2),
        ((src12[6] & 0x03) << 4) | (src12[7] >> 4),
        ((src12[7] & 0x0F) << 2) | (src12[8] >> 6),
        (src12[8] & 0x3F),
        (src12[9] >> 2),
        ((src12[9] & 0x03) << 4) | (src12[10] >> 4),
        ((src12[10] & 0x0F) << 2) | (src12[11] >> 6),
        (src12[11] & 0x3F),
    ];
    const v: B64Vec16u8 = idx;
    const v_26: B64Vec16u8 = @splat(26 as u8);
    const v_52: B64Vec16u8 = @splat(52 as u8);
    const v_62_i: B64Vec16u8 = @splat(62 as u8);
    const v_off_upper: B64Vec16u8 = @splat(65 as u8);
    const v_off_lower: B64Vec16u8 = @splat(71 as u8);
    const v_off_digit: B64Vec16u8 = @splat(252 as u8); // wrapping -4 mod 256

    const is_upper = v < v_26;
    const is_lower = (v >= v_26) & (v < v_52);
    const is_digit = (v >= v_52) & (v < v_62_i);
    const is_62 = v == v_62_i;

    const c62: u8 = if (url_safe) (45 as u8) else (43 as u8);
    const c63: u8 = if (url_safe) (95 as u8) else (47 as u8);
    const v_c62: B64Vec16u8 = @splat(c62);

    let out_v: B64Vec16u8 = @splat(c63);
    out_v = @select(u8, is_62, v_c62, out_v);
    out_v = @select(u8, is_digit, v +% v_off_digit, out_v);
    out_v = @select(u8, is_lower, v +% v_off_lower, out_v);
    out_v = @select(u8, is_upper, v +% v_off_upper, out_v);

    const out_arr: [16]u8 = out_v;
    @memcpy(dst16[0..16], &out_arr);
}

/// 16-lane SIMD decoder fast-path: validates & decodes 16 Base64 ASCII bytes into 12 raw bytes.
/// Returns `false` if any byte is outside the selected alphabet (including `'='`).
pub fn simdDecodeChunk16To12(src16: []const u8, dst12: []u8, url_safe: bool): bool {
    let raw: [16]u8 = [
        src16[0], src16[1], src16[2], src16[3],
        src16[4], src16[5], src16[6], src16[7],
        src16[8], src16[9], src16[10], src16[11],
        src16[12], src16[13], src16[14], src16[15],
    ];
    const v: B64Vec16u8 = raw;
    const v_A: B64Vec16u8 = @splat(65 as u8);
    const v_Z: B64Vec16u8 = @splat(90 as u8);
    const v_a: B64Vec16u8 = @splat(97 as u8);
    const v_z: B64Vec16u8 = @splat(122 as u8);
    const v_0: B64Vec16u8 = @splat(48 as u8);
    const v_9: B64Vec16u8 = @splat(57 as u8);
    const c62: u8 = if (url_safe) (45 as u8) else (43 as u8);
    const c63: u8 = if (url_safe) (95 as u8) else (47 as u8);
    const v_c62: B64Vec16u8 = @splat(c62);
    const v_c63: B64Vec16u8 = @splat(c63);

    const is_upper = (v >= v_A) & (v <= v_Z);
    const is_lower = (v >= v_a) & (v <= v_z);
    const is_digit = (v >= v_0) & (v <= v_9);
    const is_62 = v == v_c62;
    const is_63 = v == v_c63;

    const valid_mask: u16 = @as(u16, @bitCast(is_upper)) |
        @as(u16, @bitCast(is_lower)) |
        @as(u16, @bitCast(is_digit)) |
        @as(u16, @bitCast(is_62)) |
        @as(u16, @bitCast(is_63));
    if (valid_mask != 0xFFFF) return false;

    const v_62_val: B64Vec16u8 = @splat(62 as u8);
    const v_add_digit: B64Vec16u8 = @splat(4 as u8);
    const v_sub_lower: B64Vec16u8 = @splat(71 as u8);
    const v_sub_upper: B64Vec16u8 = @splat(65 as u8);

    let sextet_v: B64Vec16u8 = @splat(63 as u8);
    sextet_v = @select(u8, is_62, v_62_val, sextet_v);
    sextet_v = @select(u8, is_digit, v +% v_add_digit, sextet_v);
    sextet_v = @select(u8, is_lower, v -% v_sub_lower, sextet_v);
    sextet_v = @select(u8, is_upper, v -% v_sub_upper, sextet_v);

    const s: [16]u8 = sextet_v;
    dst12[0] = (s[0] << 2) | (s[1] >> 4);
    dst12[1] = (s[1] << 4) | (s[2] >> 2);
    dst12[2] = (s[2] << 6) | s[3];
    dst12[3] = (s[4] << 2) | (s[5] >> 4);
    dst12[4] = (s[5] << 4) | (s[6] >> 2);
    dst12[5] = (s[6] << 6) | s[7];
    dst12[6] = (s[8] << 2) | (s[9] >> 4);
    dst12[7] = (s[9] << 4) | (s[10] >> 2);
    dst12[8] = (s[10] << 6) | s[11];
    dst12[9] = (s[12] << 2) | (s[13] >> 4);
    dst12[10] = (s[13] << 4) | (s[14] >> 2);
    dst12[11] = (s[14] << 6) | s[15];
    return true;
}

/// Overflow-safe encoded byte length calculator; returns `null` if the result would overflow `usize`.
pub fn encodedLenChecked(input_len: usize, variant: Base64Variant): usize? {
    const max_groups: usize = std.math.maxInt(usize) / 4;
    const full_triples = input_len / 3;
    const rem = input_len % 3;
    if (hasPadding(variant)) {
        const groups = full_triples + (if (rem > 0) (1 as usize) else (0 as usize));
        if (groups > max_groups) return null;
        return groups * 4;
    }
    if (full_triples > max_groups) return null;
    const base = full_triples * 4;
    if (rem == 0) return base;
    const extra: usize = rem + 1;
    if (base > std.math.maxInt(usize) - extra) return null;
    return base + extra;
}

/// Returns the exact number of bytes required to encode `input_len` bytes using `variant`.
pub fn encodedLen(input_len: usize, variant: Base64Variant): usize {
    return encodedLenChecked(input_len, variant) orelse std.math.maxInt(usize);
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

/// Alias matching `rust-base64::decoded_len_estimate`.
pub fn decodedLenEstimate(encoded_len: usize): usize {
    return maxDecodedLen(encoded_len);
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
        if (encoded[n - 4] == PAD_CHAR || encoded[n - 3] == PAD_CHAR) {
            return err.InvalidPadding;
        }
        let result = (n / 4) * 3;
        if (encoded[n - 2] == PAD_CHAR) {
            if (encoded[n - 1] != PAD_CHAR) {
                return err.InvalidPadding;
            }
            result -= 2;
        } else if (encoded[n - 1] == PAD_CHAR) {
            result -= 1;
        }
        return result;
    }
    if (encoded[n - 1] == PAD_CHAR) {
        return err.InvalidPadding;
    }
    return maxDecodedLen(n);
}

/// Pure scalar reference encoder (used for 0-diff SIMD verification).
pub fn encodeToSliceScalar(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let out_len = encodedLenChecked(src.len, variant) orelse return err.DestinationTooSmall;
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

/// Encodes `src` into `dst` using `variant` with 16-lane SIMD fast-path (12B -> 16B).
pub fn encodeToSlice(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    let out_len = encodedLenChecked(src.len, variant) orelse return err.DestinationTooSmall;
    if (dst.len < out_len) {
        return err.DestinationTooSmall;
    }
    if (src.len == 0) {
        return dst[0..0];
    }

    const url_safe = isUrlSafeVariant(variant);
    let alphabet = alphabetFor(variant);
    let pad = hasPadding(variant);
    let in_idx: usize = 0;
    let out_idx: usize = 0;

    // 16-lane SIMD fast-path: 12 raw bytes -> 16 Base64 ASCII bytes
    while (in_idx + 12 <= src.len) {
        simdEncodeChunk12To16(src[in_idx .. in_idx + 12], dst[out_idx .. out_idx + 16], url_safe);
        in_idx += 12;
        out_idx += 16;
    }

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

/// Zero-allocation convenience alias (`rust-base64::encode_slice` / `encodeInto` style).
pub fn encodeInto(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    return encodeToSlice(dst, src, variant);
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

/// Pure scalar reference decoder (used for 0-diff SIMD verification).
pub fn decodeToSliceScalar(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
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

/// Decodes `src` into `dst` using `variant` with 16-lane SIMD fast-path (16B -> 12B)
/// and strict RFC 4648 §3.5 / `rust-base64` `RequireCanonical` validation.
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

    const url_safe = isUrlSafeVariant(variant);
    let table = decodeTableFor(variant);
    let out_idx: usize = 0;
    let in_idx: usize = 0;

    // 16-lane SIMD fast-path for complete non-padded 16-char chunks before the final quantum.
    let limit = if (n >= 4) { n - 4 } else { 0 };
    while (in_idx + 16 <= limit && out_idx + 12 <= dst.len) {
        if (!simdDecodeChunk16To12(src[in_idx .. in_idx + 16], dst[out_idx .. out_idx + 12], url_safe)) {
            break; // Fallback to scalar loop below for precise InvalidPadding vs InvalidCharacter error
        }
        in_idx += 16;
        out_idx += 12;
    }

    // Scalar loop for remaining full 4-char quanta before the final quantum.
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

/// Zero-allocation convenience alias (`rust-base64::decode_slice` / `decodeInto` style).
pub fn decodeInto(dst: []u8, src: []const u8, variant: Base64Variant): Base64Error![]u8 {
    return decodeToSlice(dst, src, variant);
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
    let out_len = encodedLenChecked(src.len, variant) orelse return err.OutOfMemory;
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
