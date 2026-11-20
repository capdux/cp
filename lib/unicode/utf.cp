// Source: bun/src/unicode/uucode_lib/src/utf8.cp -> cp/lib/unicode/utf.cp (UTF-8 / UTF-16 codecs & validation)

const std = @import("std");

pub const REPLACEMENT_CHAR: u32 = 0xFFFD;

pub const UnicodeError = error {
    InvalidUtf8,
    InvalidUtf16,
    UnpairedSurrogate,
    InvalidCodepoint,
    BufferTooSmall,
    OutOfMemory,
};

pub struct Utf8Step {
    pub codepoint: u32 = 0,
    pub len: u8 = 0,
    pub valid: bool = false,
}

/// Returns the expected UTF-8 byte sequence length (1..4) for a leading byte, or 0 if invalid.
pub fn utf8SequenceLength(lead: u8): u8 {
    if (lead < 0x80) {
        return 1;
    }
    if (lead >= 0xC2 && lead <= 0xDF) {
        return 2;
    }
    if (lead >= 0xE0 && lead <= 0xEF) {
        return 3;
    }
    if (lead >= 0xF0 && lead <= 0xF4) {
        return 4;
    }
    return 0;
}

/// Lossy single-step UTF-8 decoder for internal iterators (falls back to U+FFFD on invalid byte).
pub fn decodeUtf8Lossy(bytes: []const u8, offset: usize): Utf8Step {
    if (offset >= bytes.len) {
        return Utf8Step{ codepoint: 0, len: 0, valid: false };
    }
    const b0 = bytes[offset];
    if (b0 < 0x80) {
        return Utf8Step{ codepoint: b0 as u32, len: 1, valid: true };
    }
    const seq_len = utf8SequenceLength(b0);
    if (seq_len == 0 || offset + (seq_len as usize) > bytes.len) {
        return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
    }

    if (seq_len == 2) {
        const b1 = bytes[offset + 1];
        if ((b1 & 0xC0) != 0x80) {
            return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
        }
        const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
        return Utf8Step{ codepoint: cp, len: 2, valid: true };
    }

    if (seq_len == 3) {
        const b1 = bytes[offset + 1];
        const b2 = bytes[offset + 2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
            return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
        }
        if ((b0 == 0xE0 && b1 < 0xA0) || (b0 == 0xED && b1 >= 0xA0)) {
            return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
        }
        const cp: u32 = (((b0 & 0x0F) as u32) << 12)
            | (((b1 & 0x3F) as u32) << 6)
            | ((b2 & 0x3F) as u32);
        return Utf8Step{ codepoint: cp, len: 3, valid: true };
    }

    const b1 = bytes[offset + 1];
    const b2 = bytes[offset + 2];
    const b3 = bytes[offset + 3];
    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
        return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
    }
    if ((b0 == 0xF0 && b1 < 0x90) || (b0 == 0xF4 && b1 >= 0x90)) {
        return Utf8Step{ codepoint: REPLACEMENT_CHAR, len: 1, valid: false };
    }
    const cp: u32 = (((b0 & 0x07) as u32) << 18)
        | (((b1 & 0x3F) as u32) << 12)
        | (((b2 & 0x3F) as u32) << 6)
        | ((b3 & 0x3F) as u32);
    return Utf8Step{ codepoint: cp, len: 4, valid: true };
}

/// Strict single-step UTF-8 decoder returning `UnicodeError.InvalidUtf8` on malformed sequences.
pub fn decodeUtf8Step(bytes: []const u8, offset: usize): Utf8Step ! UnicodeError {
    if (offset >= bytes.len) {
        return UnicodeError.InvalidUtf8;
    }
    const step = decodeUtf8Lossy(bytes, offset);
    if (!step.valid) {
        return UnicodeError.InvalidUtf8;
    }
    return step;
}

/// Validates whether `bytes` is well-formed UTF-8 (RFC 3629), with fast ASCII prefix skip.
pub fn isValidUtf8(bytes: []const u8): bool {
    let i: usize = 0;
    while (i + 8 <= bytes.len) {
        const w = (bytes[i] | bytes[i + 1] | bytes[i + 2] | bytes[i + 3]
            | bytes[i + 4] | bytes[i + 5] | bytes[i + 6] | bytes[i + 7]);
        if ((w & 0x80) == 0) {
            i += 8;
        } else {
            break;
        }
    }
    while (i < bytes.len) {
        const b = bytes[i];
        if (b < 0x80) {
            i += 1;
        } else {
            const step = decodeUtf8Lossy(bytes, i);
            if (!step.valid) {
                return false;
            }
            i += (step.len as usize);
        }
    }
    return true;
}

/// Counts Unicode codepoints in `bytes` (treats malformed bytes as single replacement steps).
pub fn countCodepoints(bytes: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        const step = decodeUtf8Lossy(bytes, i);
        const adv: usize = if (step.len > 0) (step.len as usize) else 1;
        i += adv;
        count += 1;
    }
    return count;
}

/// Encodes a Unicode scalar value `cp` into `out` and returns the number of bytes written (1..4).
pub fn encodeUtf8(cp: u32, out: []u8): usize ! UnicodeError {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return UnicodeError.InvalidCodepoint;
    }
    if (cp < 0x80) {
        if (out.len < 1) {
            return UnicodeError.BufferTooSmall;
        }
        out[0] = @intCast(cp);
        return 1;
    }
    if (cp < 0x800) {
        if (out.len < 2) {
            return UnicodeError.BufferTooSmall;
        }
        out[0] = @intCast(0xC0 | (cp >> 6));
        out[1] = @intCast(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        if (out.len < 3) {
            return UnicodeError.BufferTooSmall;
        }
        out[0] = @intCast(0xE0 | (cp >> 12));
        out[1] = @intCast(0x80 | ((cp >> 6) & 0x3F));
        out[2] = @intCast(0x80 | (cp & 0x3F));
        return 3;
    }
    if (out.len < 4) {
        return UnicodeError.BufferTooSmall;
    }
    out[0] = @intCast(0xF0 | (cp >> 18));
    out[1] = @intCast(0x80 | ((cp >> 12) & 0x3F));
    out[2] = @intCast(0x80 | ((cp >> 6) & 0x3F));
    out[3] = @intCast(0x80 | (cp & 0x3F));
    return 4;
}

/// Converts a valid UTF-8 byte slice to newly allocated UTF-16 code units.
pub fn utf8ToUtf16(alloc: std.mem.Allocator, bytes: []const u8): ([]u16) ! UnicodeError {
    let unit_count: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        const step = decodeUtf8Step(bytes, i)?;
        i += (step.len as usize);
        unit_count += if (step.codepoint >= 0x10000) 2 else 1;
    }

    let out = alloc.alloc(u16, unit_count) catch return UnicodeError.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < bytes.len) {
        const step = decodeUtf8Lossy(bytes, i);
        i += (step.len as usize);
        const cp = step.codepoint;
        if (cp < 0x10000) {
            out[pos] = @intCast(cp);
            pos += 1;
        } else {
            const adj = cp - 0x10000;
            out[pos] = @intCast(0xD800 + (adj >> 10));
            out[pos + 1] = @intCast(0xDC00 + (adj & 0x3FF));
            pos += 2;
        }
    }
    return out;
}

/// Converts a UTF-16 code unit slice to newly allocated UTF-8 bytes, rejecting unpaired surrogates.
pub fn utf16ToUtf8(alloc: std.mem.Allocator, units: []const u16): ([]u8) ! UnicodeError {
    let byte_count: usize = 0;
    let i: usize = 0;
    while (i < units.len) {
        const u = units[i];
        if (u >= 0xD800 && u <= 0xDBFF) {
            if (i + 1 >= units.len) {
                return UnicodeError.UnpairedSurrogate;
            }
            const low = units[i + 1];
            if (low < 0xDC00 || low > 0xDFFF) {
                return UnicodeError.UnpairedSurrogate;
            }
            byte_count += 4;
            i += 2;
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            return UnicodeError.UnpairedSurrogate;
        } else if (u < 0x80) {
            byte_count += 1;
            i += 1;
        } else if (u < 0x800) {
            byte_count += 2;
            i += 1;
        } else {
            byte_count += 3;
            i += 1;
        }
    }

    let out = alloc.alloc(u8, byte_count) catch return UnicodeError.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < units.len) {
        const u = units[i];
        if (u >= 0xD800 && u <= 0xDBFF) {
            const high: u32 = (u as u32) - 0xD800;
            const low: u32 = (units[i + 1] as u32) - 0xDC00;
            const cp: u32 = (0x10000 as u32) + ((high << 10) | low);
            const n = encodeUtf8(cp, out[pos..]) catch {
                alloc.free(out);
                return UnicodeError.InvalidUtf16;
            };
            pos += n;
            i += 2;
        } else {
            const n = encodeUtf8(u as u32, out[pos..]) catch {
                alloc.free(out);
                return UnicodeError.InvalidUtf16;
            };
            pos += n;
            i += 1;
        }
    }
    return out;
}
