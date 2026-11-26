// @cap/cesu8 — Zero-heap CESU-8 & Java Modified UTF-8 <-> UTF-8 transcoder in pure CP
// Clean-room MIT implementation of crates.io `cesu8` (#789, 151.7M downloads).

/// Encodes standard UTF-8 bytes into CESU-8 (4-byte astral codepoints become two 3-byte UTF-16 surrogates).
/// If `modified_nul` is true (Java MUTF-8), NUL (0x00) is encoded as 2-byte `0xC0 0x80`.
/// Returns bytes written into `out`, or `-1` on invalid UTF-8 input.
pub fn toCesu8(utf8: []const u8, modified_nul: bool, out: []u8): i64 {
    let i: usize = 0;
    let p: usize = 0;
    while (i < utf8.len) {
        const b0 = utf8[i];
        if (b0 == 0 && modified_nul) {
            if (p + 1 >= out.len) { return -1; }
            out[p] = 0xC0;
            out[p + 1] = 0x80;
            p += 2;
            i += 1;
        } else if (b0 < 0x80) {
            if (p >= out.len) { return -1; }
            out[p] = b0;
            p += 1;
            i += 1;
        } else if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= utf8.len || p + 1 >= out.len) { return -1; }
            out[p] = b0;
            out[p + 1] = utf8[i + 1];
            p += 2;
            i += 2;
        } else if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 >= utf8.len || p + 2 >= out.len) { return -1; }
            out[p] = b0;
            out[p + 1] = utf8[i + 1];
            out[p + 2] = utf8[i + 2];
            p += 3;
            i += 3;
        } else if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 3 >= utf8.len || p + 5 >= out.len) { return -1; }
            const b1 = utf8[i + 1];
            const b2 = utf8[i + 2];
            const b3 = utf8[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
                return -1;
            }
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                return -1;
            }
            const sub = cp - 0x10000;
            const hi: u32 = 0xD800 + (sub >> 10);
            const lo: u32 = 0xDC00 + (sub & 0x3FF);
            out[p] = (0xE0 | (hi >> 12)) as u8;
            out[p + 1] = (0x80 | ((hi >> 6) & 0x3F)) as u8;
            out[p + 2] = (0x80 | (hi & 0x3F)) as u8;
            out[p + 3] = (0xE0 | (lo >> 12)) as u8;
            out[p + 4] = (0x80 | ((lo >> 6) & 0x3F)) as u8;
            out[p + 5] = (0x80 | (lo & 0x3F)) as u8;
            p += 6;
            i += 4;
        } else {
            return -1;
        }
    }
    return p as i64;
}

/// Decodes CESU-8 (or Java MUTF-8 when `modified_nul` is true) into standard UTF-8.
/// Returns bytes written into `out`, or `-1` on invalid CESU-8 sequence.
pub fn fromCesu8(cesu: []const u8, modified_nul: bool, out: []u8): i64 {
    let i: usize = 0;
    let p: usize = 0;
    while (i < cesu.len) {
        const b0 = cesu[i];
        if (b0 < 0x80) {
            if (p >= out.len) { return -1; }
            out[p] = b0;
            p += 1;
            i += 1;
        } else if (modified_nul && b0 == 0xC0 && i + 1 < cesu.len && cesu[i + 1] == 0x80) {
            if (p >= out.len) { return -1; }
            out[p] = 0;
            p += 1;
            i += 2;
        } else if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= cesu.len || p + 1 >= out.len) { return -1; }
            const b1 = cesu[i + 1];
            if ((b1 & 0xC0) != 0x80) { return -1; }
            out[p] = b0;
            out[p + 1] = b1;
            p += 2;
            i += 2;
        } else if (b0 == 0xED && i + 5 < cesu.len && cesu[i + 1] >= 0xA0 && cesu[i + 1] <= 0xAF) {
            // High surrogate 0xD800..0xDBFF followed by low surrogate 0xDC00..0xDFFF
            const b1 = cesu[i + 1];
            const b2 = cesu[i + 2];
            const b3 = cesu[i + 3];
            const b4 = cesu[i + 4];
            const b5 = cesu[i + 5];
            if ((b2 & 0xC0) != 0x80 || b3 != 0xED || b4 < 0xB0 || b4 > 0xBF || (b5 & 0xC0) != 0x80) {
                return -1;
            }
            const hi: u32 = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            const lo: u32 = (((b3 & 0x0F) as u32) << 12) | (((b4 & 0x3F) as u32) << 6) | ((b5 & 0x3F) as u32);
            const cp: u32 = 0x10000 + ((hi - 0xD800) << 10) + (lo - 0xDC00);
            if (p + 3 >= out.len) { return -1; }
            out[p] = (0xF0 | (cp >> 18)) as u8;
            out[p + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
            out[p + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
            out[p + 3] = (0x80 | (cp & 0x3F)) as u8;
            p += 4;
            i += 6;
        } else if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 >= cesu.len || p + 2 >= out.len) { return -1; }
            const b1 = cesu[i + 1];
            const b2 = cesu[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { return -1; }
            // Reject unpaired surrogate
            if (b0 == 0xED && b1 >= 0xA0) { return -1; }
            out[p] = b0;
            out[p + 1] = b1;
            out[p + 2] = b2;
            p += 3;
            i += 3;
        } else {
            return -1;
        }
    }
    return p as i64;
}

/// Returns `true` if `utf8` contains no 4-byte astral sequences (and no NUL if `modified_nul`),
/// meaning it is already identical in CESU-8 / MUTF-8 without transcoding.
pub fn isValidCesu8AsciiOrBmp(utf8: []const u8, modified_nul: bool): bool {
    let i: usize = 0;
    while (i < utf8.len) {
        const b = utf8[i];
        if (b == 0 && modified_nul) { return false; }
        if (b >= 0xF0) { return false; }
        i += 1;
    }
    return true;
}

export {
    toCesu8,
    fromCesu8,
    isValidCesu8AsciiOrBmp,
};
