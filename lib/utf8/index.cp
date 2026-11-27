// Source: cp/lib/utf8/index.cp (Unified Public Exports for cap:utf8)
// Strict RFC 3629 UTF-8 validation, decoding, encoding, and char-boundary foundation.

pub struct DecodedChar {
    codepoint: u32,
    len: usize,
    valid: bool,
}

fn lu8IsCont(b: u8): bool {
    return (b & 0xC0) == 0x80;
}

fn lu8IsValidScalar(cp: u32): bool {
    return cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);
}

/// Returns the expected UTF-8 sequence length (1..=4) for a valid lead byte, or 0 if invalid.
pub fn utf8SeqLen(lead_byte: u8): usize {
    if ((lead_byte & 0x80) == 0) return 1;
    if (lead_byte >= 0xC2 && lead_byte <= 0xDF) return 2;
    if (lead_byte >= 0xE0 && lead_byte <= 0xEF) return 3;
    if (lead_byte >= 0xF0 && lead_byte <= 0xF4) return 4;
    return 0;
}

/// Decodes one UTF-8 scalar value starting at `data[offset]`.
/// Strictly rejects overlong encodings, UTF-16 surrogates (U+D800..=U+DFFF),
/// codepoints > U+10FFFF, stray continuation bytes, and truncated tails.
pub fn decodeCodepoint(data: []const u8, offset: usize): DecodedChar {
    if (offset >= data.len) {
        return DecodedChar{ codepoint: 0xFFFD, len: 0, valid: false };
    }

    const b0: u8 = data[offset];
    if ((b0 & 0x80) == 0) {
        return DecodedChar{ codepoint: u32(b0), len: 1, valid: true };
    }

    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (offset + 2 > data.len) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const b1: u8 = data[offset + 1];
        if (!lu8IsCont(b1)) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const cp: u32 = (u32(b0 & 0x1F) << 6) | u32(b1 & 0x3F);
        return DecodedChar{ codepoint: cp, len: 2, valid: true };
    }

    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (offset + 3 > data.len) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const b1: u8 = data[offset + 1];
        const b2: u8 = data[offset + 2];
        if (!lu8IsCont(b1) || !lu8IsCont(b2)) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        if (b0 == 0xE0 && b1 < 0xA0) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        if (b0 == 0xED && b1 >= 0xA0) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const cp: u32 = (u32(b0 & 0x0F) << 12) | (u32(b1 & 0x3F) << 6) | u32(b2 & 0x3F);
        return DecodedChar{ codepoint: cp, len: 3, valid: true };
    }

    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (offset + 4 > data.len) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const b1: u8 = data[offset + 1];
        const b2: u8 = data[offset + 2];
        const b3: u8 = data[offset + 3];
        if (!lu8IsCont(b1) || !lu8IsCont(b2) || !lu8IsCont(b3)) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        if (b0 == 0xF0 && b1 < 0x90) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        if (b0 == 0xF4 && b1 >= 0x90) {
            return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
        }
        const cp: u32 = (u32(b0 & 0x07) << 18) | (u32(b1 & 0x3F) << 12) | (u32(b2 & 0x3F) << 6) | u32(b3 & 0x3F);
        return DecodedChar{ codepoint: cp, len: 4, valid: true };
    }

    return DecodedChar{ codepoint: 0xFFFD, len: 1, valid: false };
}

/// Validates that `data` is strictly well-formed RFC 3629 UTF-8.
pub fn isValidUtf8(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) {
        while (i + 8 <= data.len) : (i += 8) {
            const acc: u8 = data[i] | data[i + 1] | data[i + 2] | data[i + 3] | data[i + 4] | data[i + 5] | data[i + 6] | data[i + 7];
            if ((acc & 0x80) != 0) break;
        }
        if (i >= data.len) break;
        if ((data[i] & 0x80) == 0) {
            i += 1;
            continue;
        }
        const d = decodeCodepoint(data, i);
        if (!d.valid) return false;
        i += d.len;
    }
    return true;
}

/// Encodes a valid Unicode scalar value into `out` and returns the number of bytes written (1..=4).
/// Returns 0 if `cp` is invalid (surrogate or > U+10FFFF) or if `out` is too small.
pub fn encodeCodepoint(cp: u32, out: []u8): usize {
    if (!lu8IsValidScalar(cp)) return 0;

    if (cp <= 0x7F) {
        if (out.len < 1) return 0;
        out[0] = u8(cp);
        return 1;
    }
    if (cp <= 0x7FF) {
        if (out.len < 2) return 0;
        out[0] = 0xC0 | u8(cp >> 6);
        out[1] = 0x80 | u8(cp & 0x3F);
        return 2;
    }
    if (cp <= 0xFFFF) {
        if (out.len < 3) return 0;
        out[0] = 0xE0 | u8(cp >> 12);
        out[1] = 0x80 | u8((cp >> 6) & 0x3F);
        out[2] = 0x80 | u8(cp & 0x3F);
        return 3;
    }
    if (out.len < 4) return 0;
    out[0] = 0xF0 | u8(cp >> 18);
    out[1] = 0x80 | u8((cp >> 12) & 0x3F);
    out[2] = 0x80 | u8((cp >> 6) & 0x3F);
    out[3] = 0x80 | u8(cp & 0x3F);
    return 4;
}

/// Encodes `cp` into `out`; if `cp` is an invalid scalar (surrogate or > U+10FFFF), writes U+FFFD.
/// Returns 0 only if `out` is too small for the encoded sequence.
pub fn encodeCodepointLossy(cp: u32, out: []u8): usize {
    if (lu8IsValidScalar(cp)) {
        return encodeCodepoint(cp, out);
    }
    if (out.len < 3) return 0;
    out[0] = 0xEF;
    out[1] = 0xBF;
    out[2] = 0xBD;
    return 3;
}

/// Counts the number of UTF-8 codepoints (or single invalid bytes) in `data`.
pub fn countCodepoints(data: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        if ((data[i] & 0x80) == 0) {
            i += 1;
            count += 1;
            continue;
        }
        const d = decodeCodepoint(data, i);
        i += if (d.len > 0) d.len else 1;
        count += 1;
    }
    return count;
}

/// Returns true if `idx` lies on a UTF-8 character boundary in `data` (including 0 and `data.len`).
pub fn isCharBoundary(data: []const u8, idx: usize): bool {
    if (idx == 0 || idx == data.len) return true;
    if (idx > data.len) return false;
    return !lu8IsCont(data[idx]);
}

/// Returns the largest index `<= idx` (clamped to `data.len`) that lies on a UTF-8 character boundary.
pub fn floorCharBoundary(data: []const u8, idx: usize): usize {
    if (idx >= data.len) return data.len;
    let cur: usize = idx;
    while (cur > 0 && lu8IsCont(data[cur])) {
        cur -= 1;
    }
    return cur;
}
