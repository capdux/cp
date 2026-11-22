// Source: Absorbed from /data/dev/bun/src/string/wtf.cp | Synced: 2026-11-22T12:49:17Z
// lib/string/utf.cp
// High-speed UTF-8 / UTF-16 / WTF-8 codecs & East-Asian / ANSI visible display width
// Absorbed from bun/src/string/immutable/unicode.zig, wtf.zig, visible.zig & unicode/uucode/

import { SimdScanner } from "./simdScan.cp";
const std = @import("std");

pub const REPLACEMENT_CHAR: u32 = 0xFFFD;

pub struct DecodeResult {
    codepoint: u32 = 0xFFFD;
    bytes_consumed: u8 = 1;
    valid: bool = false;
}

pub struct Utf8 {
    // Returns 1..4 for valid UTF-8 leading byte, or 0 for invalid / continuation byte
    pub static fn sequenceLength(lead: u8): u8 {
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

    // Decodes one UTF-8 sequence starting at `offset` (with 0xFFFD replacement fallback)
    pub static fn decodeNext(input: []const u8, offset: usize): DecodeResult {
        if (offset >= input.len) {
            return DecodeResult{ codepoint: 0, bytes_consumed: 0, valid: false };
        }
        const b0 = input[offset];
        if (b0 < 0x80) {
            return DecodeResult{
                codepoint: b0 as u32,
                bytes_consumed: 1,
                valid: true,
            };
        }
        const seq_len = Utf8.sequenceLength(b0);
        if (seq_len == 0 || offset + (seq_len as usize) > input.len) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }

        if (seq_len == 2) {
            const b1 = input[offset + 1];
            if ((b1 & 0xC0) != 0x80) {
                return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            }
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return DecodeResult{ codepoint: cp, bytes_consumed: 2, valid: true };
        }

        if (seq_len == 3) {
            const b1 = input[offset + 1];
            const b2 = input[offset + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
                return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            }
            // Check overlong (E0 followed by < A0) and UTF-16 surrogates (ED followed by >= A0)
            if ((b0 == 0xE0 && b1 < 0xA0) || (b0 == 0xED && b1 >= 0xA0)) {
                return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            }
            const cp: u32 = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            return DecodeResult{ codepoint: cp, bytes_consumed: 3, valid: true };
        }

        // seq_len == 4
        const b1 = input[offset + 1];
        const b2 = input[offset + 2];
        const b3 = input[offset + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }
        if ((b0 == 0xF0 && b1 < 0x90) || (b0 == 0xF4 && b1 >= 0x90)) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }
        const cp: u32 = (((b0 & 0x07) as u32) << 18)
            | (((b1 & 0x3F) as u32) << 12)
            | (((b2 & 0x3F) as u32) << 6)
            | ((b3 & 0x3F) as u32);
        return DecodeResult{ codepoint: cp, bytes_consumed: 4, valid: true };
    }

    // Fast SIMD ASCII skip + full RFC 3629 UTF-8 validation
    pub static fn isValid(input: []const u8): bool {
        if (first_non_ascii := SimdScanner.firstNonAsciiIndex(input)) {
            let pos = first_non_ascii;
            while (pos < input.len) {
                const b = input[pos];
                if (b < 0x80) {
                    pos += 1;
                } else {
                    const dec_res = Utf8.decodeNext(input, pos);
                    if (!dec_res.valid) {
                        return false;
                    }
                    pos += (dec_res.bytes_consumed as usize);
                }
            }
            return true;
        }
        return true;
    }

    // Counts Unicode scalar values in valid UTF-8 slice
    pub static fn codepointCount(input: []const u8): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            if ((input[i] & 0xC0) != 0x80) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    // Encodes a single Unicode codepoint into `out[0..4]`, returning number of bytes written (0 if invalid)
    pub static fn encodeCodepoint(cp: u32, out: []u8): usize {
        if (cp <= 0x7F) {
            if (out.len < 1) {
                return 0;
            }
            out[0] = cp as u8;
            return 1;
        }
        if (cp <= 0x7FF) {
            if (out.len < 2) {
                return 0;
            }
            out[0] = (0xC0 | (cp >> 6)) as u8;
            out[1] = (0x80 | (cp & 0x3F)) as u8;
            return 2;
        }
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            return 0;
        }
        if (cp <= 0xFFFF) {
            if (out.len < 3) {
                return 0;
            }
            out[0] = (0xE0 | (cp >> 12)) as u8;
            out[1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[2] = (0x80 | (cp & 0x3F)) as u8;
            return 3;
        }
        if (cp <= 0x10FFFF) {
            if (out.len < 4) {
                return 0;
            }
            out[0] = (0xF0 | (cp >> 18)) as u8;
            out[1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
            out[2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[3] = (0x80 | (cp & 0x3F)) as u8;
            return 4;
        }
        return 0;
    }

    // Returns exact UTF-8 byte length (1..4) for a valid Unicode scalar value, or 0 if invalid/surrogate
    pub static fn encodedLength(cp: u32): usize {
        if (cp <= 0x7F) { return 1; }
        if (cp <= 0x7FF) { return 2; }
        if (cp >= 0xD800 && cp <= 0xDFFF) { return 0; }
        if (cp <= 0xFFFF) { return 3; }
        if (cp <= 0x10FFFF) { return 4; }
        return 0;
    }

    // Strict UTF-8 sequence decoder that returns an error on any invalid byte or truncated sequence
    pub static fn decodeNextStrict(input: []const u8, offset: usize): !DecodeResult {
        if (offset >= input.len) {
            return error.EndOfInput;
        }
        const res = Utf8.decodeNext(input, offset);
        if (!res.valid) {
            return error.InvalidUtf8;
        }
        return res;
    }

    // Copies `input` into `out`, replacing any invalid UTF-8 byte sequences with U+FFFD
    pub static fn replaceInvalid(input: []const u8, out: []u8): usize {
        let pos: usize = 0;
        let out_pos: usize = 0;
        while (pos < input.len) {
            const dec_res = Utf8.decodeNext(input, pos);
            const written = Utf8.encodeCodepoint(dec_res.codepoint, out[out_pos..out.len]);
            if (written == 0) {
                break;
            }
            out_pos += written;
            pos += (dec_res.bytes_consumed as usize);
        }
        return out_pos;
    }

    // Computes exact UTF-8 byte length required to encode a UTF-16LE slice (replacing lone surrogates with U+FFFD)
    pub static fn utf8LengthFromUtf16Le(input: []const u16): usize {
        return Utf16.utf8LengthFromUtf16Le(input);
    }
}

pub struct EncodeIntoResult {
    read: u32 = 0,
    written: u32 = 0,
}

/// WTF-8（允许孤立 UTF-16 代理对编码）编解码支持，移植自 Bun src/string/wtf.zig
pub struct Wtf8 {
    pub static fn byteSequenceLength(lead: u8): u8 {
        if (lead < 0x80) return 1;
        if (lead >= 0xC2 && lead <= 0xDF) return 2;
        if (lead >= 0xE0 && lead <= 0xEF) return 3;
        if (lead >= 0xF0 && lead <= 0xF4) return 4;
        return 0;
    }

    pub static fn encodeSurrogatePair(high: u16, low: u16): u32 {
        if (high < 0xD800 || high > 0xDBFF || low < 0xDC00 || low > 0xDFFF) {
            return REPLACEMENT_CHAR;
        }
        return (0x10000 as u32) + (((high as u32) - 0xD800) << 10) + ((low as u32) - 0xDC00);
    }

    pub static fn decodeSurrogatePair(cp: u32): [2]u16 {
        if (cp < 0x10000 || cp > 0x10FFFF) {
            return [0xFFFD, 0];
        }
        const adj = cp - 0x10000;
        return [(0xD800 + (adj >> 10)) as u16, (0xDC00 + (adj & 0x3FF)) as u16];
    }

    /// WTF-8 编码：允许 0xD800..0xDFFF 代理码点
    pub static fn encodeRune(cp: u32, out: []u8): usize {
        if (cp <= 0x7F) {
            if (out.len < 1) return 0;
            out[0] = cp as u8;
            return 1;
        }
        if (cp <= 0x7FF) {
            if (out.len < 2) return 0;
            out[0] = (0xC0 | (cp >> 6)) as u8;
            out[1] = (0x80 | (cp & 0x3F)) as u8;
            return 2;
        }
        if (cp <= 0xFFFF) {
            if (out.len < 3) return 0;
            out[0] = (0xE0 | (cp >> 12)) as u8;
            out[1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[2] = (0x80 | (cp & 0x3F)) as u8;
            return 3;
        }
        if (cp <= 0x10FFFF) {
            if (out.len < 4) return 0;
            out[0] = (0xF0 | (cp >> 18)) as u8;
            out[1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
            out[2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[3] = (0x80 | (cp & 0x3F)) as u8;
            return 4;
        }
        return 0;
    }

    /// WTF-8 解码：允许 ED A0..BF xx 孤立代理对
    pub static fn decodeRune(input: []const u8, offset: usize): DecodeResult {
        if (offset >= input.len) {
            return DecodeResult{ codepoint: 0, bytes_consumed: 0, valid: false };
        }
        const b0 = input[offset];
        if (b0 < 0x80) {
            return DecodeResult{ codepoint: b0 as u32, bytes_consumed: 1, valid: true };
        }
        const seq_len = Wtf8.byteSequenceLength(b0);
        if (seq_len == 0 || offset + (seq_len as usize) > input.len) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }
        if (seq_len == 2) {
            const b1 = input[offset + 1];
            if ((b1 & 0xC0) != 0x80) return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            const cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return DecodeResult{ codepoint: cp, bytes_consumed: 2, valid: true };
        }
        if (seq_len == 3) {
            const b1 = input[offset + 1];
            const b2 = input[offset + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
                return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            }
            if (b0 == 0xE0 && b1 < 0xA0) {
                return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
            }
            const cp = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            return DecodeResult{ codepoint: cp, bytes_consumed: 3, valid: true };
        }
        const b1 = input[offset + 1];
        const b2 = input[offset + 2];
        const b3 = input[offset + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }
        if ((b0 == 0xF0 && b1 < 0x90) || (b0 == 0xF4 && b1 >= 0x90)) {
            return DecodeResult{ codepoint: REPLACEMENT_CHAR, bytes_consumed: 1, valid: false };
        }
        const cp = (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32);
        return DecodeResult{ codepoint: cp, bytes_consumed: 4, valid: true };
    }

    pub static fn isValid(input: []const u8): bool {
        let pos: usize = 0;
        while (pos < input.len) {
            const decoded = Wtf8.decodeRune(input, pos);
            if (!decoded.valid) return false;
            pos += decoded.bytes_consumed as usize;
        }
        return true;
    }

    pub static fn fromUtf16(input: []const u16, out: []u8): usize {
        let i: usize = 0;
        let out_pos: usize = 0;
        while (i < input.len) {
            const unit0 = input[i] as u32;
            let cp: u32 = unit0;
            if (unit0 >= 0xD800 && unit0 <= 0xDBFF && i + 1 < input.len) {
                const unit1 = input[i + 1] as u32;
                if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                    cp = Wtf8.encodeSurrogatePair(@intCast(unit0), @intCast(unit1));
                    i += 1;
                }
            }
            const n = Wtf8.encodeRune(cp, out[out_pos..out.len]);
            if (n == 0) break;
            out_pos += n;
            i += 1;
        }
        return out_pos;
    }

    pub static fn toUtf16(input: []const u8, out: []u16): usize {
        let pos: usize = 0;
        let out_idx: usize = 0;
        while (pos < input.len) {
            const decoded = Wtf8.decodeRune(input, pos);
            const cp = decoded.codepoint;
            if (cp < 0x10000) {
                if (out_idx >= out.len) break;
                out[out_idx] = cp as u16;
                out_idx += 1;
            } else {
                if (out_idx + 1 >= out.len) break;
                const pair = Wtf8.decodeSurrogatePair(cp);
                out[out_idx] = pair[0];
                out[out_idx + 1] = pair[1];
                out_idx += 2;
            }
            pos += if (decoded.bytes_consumed > 0) (decoded.bytes_consumed as usize) else 1;
        }
        return out_idx;
    }
}

pub struct Utf16 {
    pub static fn isHighSurrogate(unit: u16): bool {
        return unit >= 0xD800 && unit <= 0xDBFF;
    }

    pub static fn isLowSurrogate(unit: u16): bool {
        return unit >= 0xDC00 && unit <= 0xDFFF;
    }

    pub static fn isSurrogate(unit: u16): bool {
        return unit >= 0xD800 && unit <= 0xDFFF;
    }

    // Returns true if `input` contains no unpaired UTF-16 surrogates
    pub static fn isValid(input: []const u16): bool {
        let i: usize = 0;
        while (i < input.len) {
            const u = input[i];
            if (Utf16.isHighSurrogate(u)) {
                if (i + 1 >= input.len || !Utf16.isLowSurrogate(input[i + 1])) {
                    return false;
                }
                i += 2;
            } else if (Utf16.isLowSurrogate(u)) {
                return false;
            } else {
                i += 1;
            }
        }
        return true;
    }

    // Computes exact UTF-8 byte length from UTF-16LE input (handles BMP, surrogate pairs, and U+FFFD fallback)
    pub static fn utf8LengthFromUtf16Le(input: []const u16): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            const unit0 = input[i] as u32;
            if (unit0 >= 0xD800 && unit0 <= 0xDBFF) {
                if (i + 1 < input.len) {
                    const unit1 = input[i + 1] as u32;
                    if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                        total += 4;
                        i += 2;
                        continue;
                    }
                }
                total += 3; // U+FFFD
                i += 1;
            } else if (unit0 >= 0xDC00 && unit0 <= 0xDFFF) {
                total += 3; // U+FFFD
                i += 1;
            } else if (unit0 <= 0x7F) {
                total += 1;
                i += 1;
            } else if (unit0 <= 0x7FF) {
                total += 2;
                i += 1;
            } else {
                total += 3;
                i += 1;
            }
        }
        return total;
    }

    // Computes exact UTF-16 code unit length from UTF-8 input (matching Bun elementLengthUTF8IntoUTF16)
    pub static fn utf16LengthFromUtf8(input: []const u8): usize {
        let u16_len: usize = 0;
        let pos: usize = 0;
        while (pos < input.len) {
            const dec_res = Utf8.decodeNext(input, pos);
            if (dec_res.codepoint >= 0x10000) {
                u16_len += 2;
            } else {
                u16_len += 1;
            }
            pos += (dec_res.bytes_consumed as usize);
        }
        return u16_len;
    }

    // Encodes UTF-8 slice into UTF-16LE buffer `out`, returning number of u16 units written
    pub static fn encodeUtf8ToUtf16Le(input: []const u8, out: []u16): usize {
        let pos: usize = 0;
        let out_idx: usize = 0;
        while (pos < input.len) {
            const dec_res = Utf8.decodeNext(input, pos);
            const cp = dec_res.codepoint;
            if (cp < 0x10000) {
                if (out_idx >= out.len) {
                    break;
                }
                out[out_idx] = cp as u16;
                out_idx += 1;
            } else {
                if (out_idx + 1 >= out.len) {
                    break;
                }
                const adj = cp - 0x10000;
                const high: u16 = (0xD800 + (adj >> 10)) as u16;
                const low: u16 = (0xDC00 + (adj & 0x3FF)) as u16;
                out[out_idx] = high;
                out[out_idx + 1] = low;
                out_idx += 2;
            }
            pos += (dec_res.bytes_consumed as usize);
        }
        return out_idx;
    }

    // Decodes UTF-16LE slice into UTF-8 buffer `out`, returning number of bytes written
    pub static fn decodeUtf16LeToUtf8(input: []const u16, out: []u8): usize {
        let i: usize = 0;
        let out_pos: usize = 0;
        while (i < input.len) {
            const unit0 = input[i] as u32;
            let cp: u32 = unit0;
            if (unit0 >= 0xD800 && unit0 <= 0xDBFF) {
                if (i + 1 < input.len) {
                    const unit1 = input[i + 1] as u32;
                    if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
                        cp = (0x10000 as u32) + ((unit0 - 0xD800) << 10) + (unit1 - 0xDC00);
                        i += 1;
                    } else {
                        cp = REPLACEMENT_CHAR;
                    }
                } else {
                    cp = REPLACEMENT_CHAR;
                }
            } else if (unit0 >= 0xDC00 && unit0 <= 0xDFFF) {
                cp = REPLACEMENT_CHAR;
            }
            const written = Utf8.encodeCodepoint(cp, out[out_pos..out.len]);
            if (written == 0) {
                break;
            }
            out_pos += written;
            i += 1;
        }
        return out_pos;
    }

    // Strict UTF-8 -> UTF-16LE encoder: returns error on invalid UTF-8 or insufficient output buffer
    pub static fn encodeUtf8ToUtf16LeStrict(input: []const u8, out: []u16): !usize {
        let pos: usize = 0;
        let out_idx: usize = 0;
        while (pos < input.len) {
            const dec_res = Utf8.decodeNext(input, pos);
            if (!dec_res.valid) {
                return error.InvalidUtf8;
            }
            const cp = dec_res.codepoint;
            if (cp < 0x10000) {
                if (out_idx >= out.len) {
                    return error.BufferTooSmall;
                }
                out[out_idx] = cp as u16;
                out_idx += 1;
            } else {
                if (out_idx + 1 >= out.len) {
                    return error.BufferTooSmall;
                }
                const adj = cp - 0x10000;
                out[out_idx] = (0xD800 + (adj >> 10)) as u16;
                out[out_idx + 1] = (0xDC00 + (adj & 0x3FF)) as u16;
                out_idx += 2;
            }
            pos += (dec_res.bytes_consumed as usize);
        }
        return out_idx;
    }

    // Strict UTF-16LE -> UTF-8 decoder: returns error on lone surrogate or insufficient output buffer
    pub static fn decodeUtf16LeToUtf8Strict(input: []const u16, out: []u8): !usize {
        let i: usize = 0;
        let out_pos: usize = 0;
        while (i < input.len) {
            const unit0 = input[i] as u32;
            let cp: u32 = unit0;
            if (unit0 >= 0xD800 && unit0 <= 0xDBFF) {
                if (i + 1 >= input.len) {
                    return error.InvalidSurrogatePair;
                }
                const unit1 = input[i + 1] as u32;
                if (unit1 < 0xDC00 || unit1 > 0xDFFF) {
                    return error.InvalidSurrogatePair;
                }
                cp = (0x10000 as u32) + ((unit0 - 0xD800) << 10) + (unit1 - 0xDC00);
                i += 1;
            } else if (unit0 >= 0xDC00 && unit0 <= 0xDFFF) {
                return error.InvalidSurrogatePair;
            }
            const written = Utf8.encodeCodepoint(cp, out[out_pos..out.len]);
            if (written == 0) {
                return error.BufferTooSmall;
            }
            out_pos += written;
            i += 1;
        }
        return out_pos;
    }

    // Allocates an exact-sized UTF-16LE buffer and encodes UTF-8 input into it
    pub static fn encodeUtf8ToUtf16LeAlloc(allocator: std.mem.Allocator, input: []const u8): ![]u16 {
        const needed = Utf16.utf16LengthFromUtf8(input);
        const buf = try allocator.alloc(u16, needed);
        const written = Utf16.encodeUtf8ToUtf16Le(input, buf);
        return buf[0..written];
    }

    // Allocates an exact-sized UTF-8 buffer and decodes UTF-16LE input into it
    pub static fn decodeUtf16LeToUtf8Alloc(allocator: std.mem.Allocator, input: []const u16): ![]u8 {
        const needed = Utf16.utf8LengthFromUtf16Le(input);
        const buf = try allocator.alloc(u8, needed);
        const written = Utf16.decodeUtf16LeToUtf8(input, buf);
        return buf[0..written];
    }
}

// Terminal & East-Asian visible display width (absorbed from bun/src/string/immutable/visible.zig & unicode/uucode)
pub struct UnicodeWidth {
    pub static fn codepointWidth(cp: u32): usize {
        if (cp == 0) {
            return 0;
        }
        // Control characters & combining marks
        if (cp < 32 || (cp >= 0x7F && cp < 0xA0)) {
            return 0;
        }
        if (cp >= 0x0300 && cp <= 0x036F) {
            return 0;
        }
        // CJK Unified Ideographs, Hangul, Kana, Fullwidth Forms & Emoji blocks
        if ((cp >= 0x1100 && cp <= 0x115F)
            || (cp >= 0x2E80 && cp <= 0xA4CF)
            || (cp >= 0xAC00 && cp <= 0xD7A3)
            || (cp >= 0xF900 && cp <= 0xFAFF)
            || (cp >= 0xFE10 && cp <= 0xFE6F)
            || (cp >= 0xFF01 && cp <= 0xFF60)
            || (cp >= 0x1F300 && cp <= 0x1FAFF))
        {
            return 2;
        }
        return 1;
    }

    // Computes visible column width, automatically stripping ANSI CSI escape codes (`\x1b[...m`)
    pub static fn visibleWidth(input: []const u8): usize {
        let width: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            // Skip ANSI escape sequence `\x1b[...m`
            if (input[i] == 0x1B && i + 1 < input.len && input[i + 1] == 91) {
                i += 2;
                while (i < input.len) {
                    const ch = input[i];
                    i += 1;
                    if ((ch >= 64 && ch <= 126)) {
                        break;
                    }
                }
            } else {
                const dec_res = Utf8.decodeNext(input, i);
                width += UnicodeWidth.codepointWidth(dec_res.codepoint);
                i += (dec_res.bytes_consumed as usize);
            }
        }
        return width;
    }
}

pub fn utf8ByteSequenceLength(lead: u8): u8 {
    return Utf8.sequenceLength(lead);
}

pub fn decodeUTF8(input: []const u8, offset: usize): DecodeResult {
    return Utf8.decodeNext(input, offset);
}

pub fn encodeUTF8(cp: u32, out: []u8): usize {
    return Utf8.encodeCodepoint(cp, out);
}

pub fn elementLengthLatin1IntoUTF8(latin1: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < latin1.len) {
        total += if (latin1[i] >= 0x80) (2 as usize) else (1 as usize);
        i += 1;
    }
    return total;
}

/// 将 Latin-1 字节流转码为 UTF-8，返回实际读取与写入字节数（对齐 Bun copyLatin1IntoUTF8）
pub fn copyLatin1IntoUTF8(out: []u8, latin1: []const u8): EncodeIntoResult {
    let read_pos: usize = 0;
    let write_pos: usize = 0;
    while (read_pos < latin1.len && write_pos < out.len) {
        const b = latin1[read_pos];
        if (b < 0x80) {
            out[write_pos] = b;
            write_pos += 1;
            read_pos += 1;
        } else {
            if (write_pos + 2 > out.len) break;
            out[write_pos] = (0xC0 | ((b as u32) >> 6)) as u8;
            out[write_pos + 1] = (0x80 | ((b as u32) & 0x3F)) as u8;
            write_pos += 2;
            read_pos += 1;
        }
    }
    return EncodeIntoResult{
        read: @intCast(read_pos),
        written: @intCast(write_pos),
    };
}

pub fn elementLengthUTF16IntoUTF8(utf16: []const u16): usize {
    return Utf16.utf8LengthFromUtf16Le(utf16);
}

/// 将 UTF-16LE 单元流转码为 UTF-8，孤立代理替换为 U+FFFD，返回实际读取单元与写入字节数
pub fn copyUTF16IntoUTF8(out: []u8, utf16: []const u16): EncodeIntoResult {
    let read_pos: usize = 0;
    let write_pos: usize = 0;
    while (read_pos < utf16.len && write_pos < out.len) {
        const unit0 = utf16[read_pos] as u32;
        if (unit0 < 0x80) {
            out[write_pos] = unit0 as u8;
            write_pos += 1;
            read_pos += 1;
        } else if (unit0 < 0x800) {
            if (write_pos + 2 > out.len) break;
            out[write_pos] = (0xC0 | (unit0 >> 6)) as u8;
            out[write_pos + 1] = (0x80 | (unit0 & 0x3F)) as u8;
            write_pos += 2;
            read_pos += 1;
        } else if (unit0 >= 0xD800 && unit0 <= 0xDBFF) {
            if (read_pos + 1 < utf16.len && utf16[read_pos + 1] >= 0xDC00 && utf16[read_pos + 1] <= 0xDFFF) {
                if (write_pos + 4 > out.len) break;
                const cp = Wtf8.encodeSurrogatePair(@intCast(unit0), utf16[read_pos + 1]);
                const n = Utf8.encodeCodepoint(cp, out[write_pos..out.len]);
                if (n == 0) break;
                write_pos += n;
                read_pos += 2;
            } else {
                if (write_pos + 3 > out.len) break;
                const n = Utf8.encodeCodepoint(REPLACEMENT_CHAR, out[write_pos..out.len]);
                if (n == 0) break;
                write_pos += n;
                read_pos += 1;
            }
        } else if (unit0 >= 0xDC00 && unit0 <= 0xDFFF) {
            if (write_pos + 3 > out.len) break;
            const n = Utf8.encodeCodepoint(REPLACEMENT_CHAR, out[write_pos..out.len]);
            if (n == 0) break;
            write_pos += n;
            read_pos += 1;
        } else {
            if (write_pos + 3 > out.len) break;
            out[write_pos] = (0xE0 | (unit0 >> 12)) as u8;
            out[write_pos + 1] = (0x80 | ((unit0 >> 6) & 0x3F)) as u8;
            out[write_pos + 2] = (0x80 | (unit0 & 0x3F)) as u8;
            write_pos += 3;
            read_pos += 1;
        }
    }
    return EncodeIntoResult{
        read: @intCast(read_pos),
        written: @intCast(write_pos),
    };
}

pub fn convertUTF8toUTF16(allocator: std.mem.Allocator, input: []const u8): ![]u16 {
    return Utf16.encodeUtf8ToUtf16LeAlloc(allocator, input);
}

pub fn convertUTF8toUTF16InBuffer(out: []u16, input: []const u8): EncodeIntoResult {
    let pos: usize = 0;
    let out_idx: usize = 0;
    while (pos < input.len) {
        const dec_res = Utf8.decodeNext(input, pos);
        const cp = dec_res.codepoint;
        const step: usize = if (dec_res.bytes_consumed > 0) (dec_res.bytes_consumed as usize) else 1;
        if (cp < 0x10000) {
            if (out_idx >= out.len) break;
            out[out_idx] = cp as u16;
            out_idx += 1;
        } else {
            if (out_idx + 1 >= out.len) break;
            const adj = cp - 0x10000;
            out[out_idx] = (0xD800 + (adj >> 10)) as u16;
            out[out_idx + 1] = (0xDC00 + (adj & 0x3FF)) as u16;
            out_idx += 2;
        }
        pos += step;
    }
    return EncodeIntoResult{
        read: @intCast(pos),
        written: @intCast(out_idx),
    };
}

pub fn convertUTF16toUTF8(allocator: std.mem.Allocator, input: []const u16): ![]u8 {
    return Utf16.decodeUtf16LeToUtf8Alloc(allocator, input);
}

pub fn convertUTF16toUTF8InBuffer(out: []u8, input: []const u16): EncodeIntoResult {
    return copyUTF16IntoUTF8(out, input);
}

pub fn withoutUTF8BOM(bytes: []const u8): []const u8 {
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return bytes[3..bytes.len];
    }
    return bytes;
}
