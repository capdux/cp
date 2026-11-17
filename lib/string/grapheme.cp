// Source: Bun src/unicode/uucode_lib/
// lib/string/grapheme.cp
// High-Speed UTF-8 Validator, Codepoint Counter & UAX #29 Extended Grapheme Cluster Analyzer
// Absorbed from Bun src/unicode/uucode_lib/ & src/simdutf_sys/

pub struct Utf8Grapheme {
    /// Validates whether `bytes` is well-formed UTF-8 according to RFC 3629.
    pub static fn isValidUtf8(bytes: []const u8): bool {
        let i: usize = 0;
        while (i < bytes.len) {
            const b0 = bytes[i];
            if (b0 < 0x80) {
                i += 1;
                continue;
            }
            if (b0 >= 0xC2 && b0 <= 0xDF) {
                if (i + 1 >= bytes.len || (bytes[i + 1] & 0xC0) != 0x80) {
                    return false;
                }
                i += 2;
                continue;
            }
            if (b0 >= 0xE0 && b0 <= 0xEF) {
                if (i + 2 >= bytes.len) { return false; }
                const b1 = bytes[i + 1];
                const b2 = bytes[i + 2];
                if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { return false; }
                if (b0 == 0xE0 && b1 < 0xA0) { return false; } // overlong
                if (b0 == 0xED && b1 >= 0xA0) { return false; } // UTF-16 surrogate
                i += 3;
                continue;
            }
            if (b0 >= 0xF0 && b0 <= 0xF4) {
                if (i + 3 >= bytes.len) { return false; }
                const b1 = bytes[i + 1];
                const b2 = bytes[i + 2];
                const b3 = bytes[i + 3];
                if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) { return false; }
                if (b0 == 0xF0 && b1 < 0x90) { return false; } // overlong
                if (b0 == 0xF4 && b1 >= 0x90) { return false; } // > U+10FFFF
                i += 4;
                continue;
            }
            return false;
        }
        return true;
    }

    /// Returns the byte length (1..4) of the UTF-8 sequence starting at `lead_byte`.
    pub static fn sequenceLength(lead_byte: u8): usize {
        if (lead_byte < 0x80) { return 1; }
        if ((lead_byte & 0xE0) == 0xC0) { return 2; }
        if ((lead_byte & 0xF0) == 0xE0) { return 3; }
        if ((lead_byte & 0xF8) == 0xF0) { return 4; }
        return 1;
    }

    /// Decodes a single Unicode scalar value (codepoint) at `bytes[offset..]`.
    pub static fn decodeCodepointAt(bytes: []const u8, offset: usize): u32 {
        if (offset >= bytes.len) { return 0; }
        const b0 = bytes[offset];
        if (b0 < 0x80) {
            return b0 as u32;
        }
        if ((b0 & 0xE0) == 0xC0 && offset + 1 < bytes.len) {
            return (((b0 & 0x1F) as u32) << 6) | ((bytes[offset + 1] & 0x3F) as u32);
        }
        if ((b0 & 0xF0) == 0xE0 && offset + 2 < bytes.len) {
            return (((b0 & 0x0F) as u32) << 12) |
                   (((bytes[offset + 1] & 0x3F) as u32) << 6) |
                   ((bytes[offset + 2] & 0x3F) as u32);
        }
        if ((b0 & 0xF8) == 0xF0 && offset + 3 < bytes.len) {
            return (((b0 & 0x07) as u32) << 18) |
                   (((bytes[offset + 1] & 0x3F) as u32) << 12) |
                   (((bytes[offset + 2] & 0x3F) as u32) << 6) |
                   ((bytes[offset + 3] & 0x3F) as u32);
        }
        return 0xFFFD;
    }

    /// Returns true if `cp` is a Unicode combining mark, variation selector, or Zero-Width Joiner (ZWJ).
    pub static fn isGraphemeExtend(cp: u32): bool {
        // Combining Diacritical Marks (0300..036F), Variation Selectors (FE00..FE0F), Skin tones (1F3FB..1F3FF)
        if (cp >= 0x0300 && cp <= 0x036F) { return true; }
        if (cp >= 0xFE00 && cp <= 0xFE0F) { return true; }
        if (cp >= 0x1F3FB && cp <= 0x1F3FF) { return true; }
        if (cp == 0x200D) { return true; } // ZWJ
        return false;
    }

    /// Counts Unicode scalar values (codepoints) in `bytes`.
    pub static fn countCodepoints(bytes: []const u8): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < bytes.len) {
            i += Utf8Grapheme.sequenceLength(bytes[i]);
            count += 1;
        }
        return count;
    }

    /// Counts user-perceived characters (UAX #29 extended grapheme clusters, handling `\r\n`, combining marks, and ZWJ sequences).
    pub static fn countGraphemes(bytes: []const u8): usize {
        let clusters: usize = 0;
        let i: usize = 0;
        let prev_was_zwj: bool = false;

        while (i < bytes.len) {
            if (bytes[i] == '\r' && i + 1 < bytes.len && bytes[i + 1] == '\n') {
                clusters += 1;
                i += 2;
                prev_was_zwj = false;
                continue;
            }
            const cp = Utf8Grapheme.decodeCodepointAt(bytes, i);
            const seq_len = Utf8Grapheme.sequenceLength(bytes[i]);
            i += seq_len;

            if (clusters == 0) {
                clusters = 1;
            } else if (!Utf8Grapheme.isGraphemeExtend(cp) && !prev_was_zwj) {
                clusters += 1;
            }
            prev_was_zwj = (cp == 0x200D);
        }

        return clusters;
    }
}
