// packages/@cap/unicode-width/src/index.cp
// Zero-allocation Unicode 15.1 East Asian Width & terminal column width engine.
// MIT clean-room pure CP implementation.

fn uwNextCodepoint(s: []const u8, pos: usize, next_pos: *usize): u32 {
    const b0 = s[pos];
    if (b0 < (0x80 as u8)) {
        next_pos.* = pos + 1;
        return b0 as u32;
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 2;
        return (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((s[pos + 1] & (0x3F as u8)) as u32);
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 3;
        return (((b0 & (0x0F as u8)) as u32) << (12 as u5)) | (((s[pos + 1] & (0x3F as u8)) as u32) << (6 as u5)) | ((s[pos + 2] & (0x3F as u8)) as u32);
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 3] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 4;
        return (((b0 & (0x07 as u8)) as u32) << (18 as u5)) | (((s[pos + 1] & (0x3F as u8)) as u32) << (12 as u5)) | (((s[pos + 2] & (0x3F as u8)) as u32) << (6 as u5)) | ((s[pos + 3] & (0x3F as u8)) as u32);
    }
    // Malformed UTF-8 byte: advance safely by 1 byte as U+FFFD (width 1).
    next_pos.* = pos + 1;
    return 0xFFFD as u32;
}

fn uwIsZeroWidth(cp: u32): bool {
    // Control chars C0 and C1 + DEL
    if (cp <= (0x1F as u32) || (cp >= (0x7F as u32) && cp <= (0x9F as u32))) { return true; }
    // Combining Diacritical Marks & Extensions
    if (cp >= (0x0300 as u32) && cp <= (0x036F as u32)) { return true; }
    if (cp >= (0x1AB0 as u32) && cp <= (0x1AFF as u32)) { return true; }
    if (cp >= (0x1DC0 as u32) && cp <= (0x1DFF as u32)) { return true; }
    if (cp >= (0x20D0 as u32) && cp <= (0x20FF as u32)) { return true; }
    if (cp >= (0xFE20 as u32) && cp <= (0xFE2F as u32)) { return true; }
    // Zero-width spaces, joiners (ZWJ 0x200D), direction marks, BOM, Variation Selectors
    if (cp >= (0x200B as u32) && cp <= (0x200F as u32)) { return true; }
    if (cp >= (0x202A as u32) && cp <= (0x202E as u32)) { return true; }
    if (cp >= (0x2060 as u32) && cp <= (0x206F as u32)) { return true; }
    if (cp >= (0xFE00 as u32) && cp <= (0xFE0F as u32)) { return true; }
    if (cp == (0xFEFF as u32)) { return true; }
    if (cp >= (0xE0100 as u32) && cp <= (0xE01EF as u32)) { return true; }
    return false;
}

fn uwIsWide(cp: u32): bool {
    if (cp < (0x1100 as u32)) { return false; }
    if (cp <= (0x115F as u32)) { return true; }
    if (cp == (0x2329 as u32) || cp == (0x232A as u32)) { return true; }
    if (cp >= (0x2E80 as u32) && cp <= (0x303E as u32)) { return true; }
    if (cp >= (0x3040 as u32) && cp <= (0x33FF as u32)) { return true; }
    if (cp >= (0x3400 as u32) && cp <= (0x4DBF as u32)) { return true; }
    if (cp >= (0x4E00 as u32) && cp <= (0xA4CF as u32)) { return true; }
    if (cp >= (0xA960 as u32) && cp <= (0xA97F as u32)) { return true; }
    if (cp >= (0xAC00 as u32) && cp <= (0xD7A3 as u32)) { return true; }
    if (cp >= (0xF900 as u32) && cp <= (0xFAFF as u32)) { return true; }
    if (cp >= (0xFE10 as u32) && cp <= (0xFE19 as u32)) { return true; }
    if (cp >= (0xFE30 as u32) && cp <= (0xFE6F as u32)) { return true; }
    if (cp >= (0xFF01 as u32) && cp <= (0xFF60 as u32)) { return true; }
    if (cp >= (0xFFE0 as u32) && cp <= (0xFFE6 as u32)) { return true; }
    if (cp >= (0x1F300 as u32) && cp <= (0x1FAFF as u32)) { return true; }
    if (cp >= (0x20000 as u32) && cp <= (0x3FFFD as u32)) { return true; }
    return false;
}

fn uwIsAmbiguous(cp: u32): bool {
    if (cp == (0x00A1 as u32) || cp == (0x00A4 as u32) || cp == (0x00A7 as u32) || cp == (0x00A8 as u32)) { return true; }
    if (cp == (0x00AA as u32) || cp == (0x00AE as u32) || cp == (0x00D7 as u32) || cp == (0x00F7 as u32)) { return true; }
    if (cp >= (0x00B0 as u32) && cp <= (0x00B4 as u32)) { return true; }
    if (cp >= (0x00B6 as u32) && cp <= (0x00BA as u32)) { return true; }
    if (cp >= (0x00BC as u32) && cp <= (0x00BF as u32)) { return true; }
    if (cp >= (0x0391 as u32) && cp <= (0x03A9 as u32)) { return true; }
    if (cp >= (0x03B1 as u32) && cp <= (0x03C9 as u32)) { return true; }
    if (cp >= (0x0410 as u32) && cp <= (0x044F as u32)) { return true; }
    if (cp >= (0x2010 as u32) && cp <= (0x2027 as u32)) { return true; }
    if (cp >= (0x2030 as u32) && cp <= (0x203E as u32)) { return true; }
    if (cp >= (0x2100 as u32) && cp <= (0x21FF as u32)) { return true; }
    if (cp >= (0x2200 as u32) && cp <= (0x22FF as u32)) { return true; }
    if (cp >= (0x2500 as u32) && cp <= (0x26FF as u32)) { return true; }
    return false;
}

/// Standard terminal display column width for Unicode code point `cp` (`0`, `1`, or `2`).
pub fn charWidth(cp: u32): usize {
    if (cp >= (0x20 as u32) && cp <= (0x7E as u32)) {
        return 1;
    }
    if (uwIsZeroWidth(cp)) {
        return 0;
    }
    if (uwIsWide(cp)) {
        return 2;
    }
    return 1;
}

/// Terminal display column width in CJK context where East Asian Ambiguous characters occupy 2 columns.
pub fn charWidthCjk(cp: u32): usize {
    const base = charWidth(cp);
    if (base == 0 || base == 2) {
        return base;
    }
    if (uwIsAmbiguous(cp)) {
        return 2;
    }
    return 1;
}

/// Compute total terminal display column width of UTF-8 string `s`.
pub fn strWidth(s: string): usize {
    let total: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        let nextPos: usize = pos + 1;
        const cp = uwNextCodepoint(s, pos, &nextPos);
        total += charWidth(cp);
        pos = nextPos;
    }
    return total;
}

/// Compute total terminal display column width of UTF-8 string `s` in CJK context.
pub fn strWidthCjk(s: string): usize {
    let total: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        let nextPos: usize = pos + 1;
        const cp = uwNextCodepoint(s, pos, &nextPos);
        total += charWidthCjk(cp);
        pos = nextPos;
    }
    return total;
}

/// Return the largest safe UTF-8 byte prefix length of `s` whose display width does not exceed `max_cols`.
/// Never splits a multi-byte UTF-8 code point.
pub fn truncateToWidth(s: string, max_cols: usize): usize {
    let cols: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        let nextPos: usize = pos + 1;
        const cp = uwNextCodepoint(s, pos, &nextPos);
        const w = charWidth(cp);
        if (cols + w > max_cols) {
            return pos;
        }
        cols += w;
        pos = nextPos;
    }
    return pos;
}

export {
    charWidth,
    charWidthCjk,
    strWidth,
    strWidthCjk,
    truncateToWidth,
};
