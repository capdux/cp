// Source: Absorbed from /data/dev/bun/src/string/immutable.zig (Unicode East Asian Display Width / wcwidth)
// Pure-CP zero-allocation terminal display column width calculator (combining=0, ASCII=1, CJK/Fullwidth/Emoji=2).

/// Decode one UTF-8 codepoint starting at `s[pos]`, advancing `pos`. Returns `0xFFFD` on invalid sequence.
fn nextCodepoint(s: []const u8, pos: *usize): u32? {
    if (pos.* >= s.len) return null;
    const b0 = s[pos.*];
    if (b0 < 0x80) {
        pos.* += 1;
        return @as(u32, b0);
    }
    if ((b0 & 0xE0) == 0xC0 && pos.* + 1 < s.len) {
        const b1 = s[pos.* + 1];
        if ((b1 & 0xC0) == 0x80) {
            pos.* += 2;
            return ((@as(u32, b0) & 0x1F) << 6) | (@as(u32, b1) & 0x3F);
        }
    } else if ((b0 & 0xF0) == 0xE0 && pos.* + 2 < s.len) {
        const b1 = s[pos.* + 1];
        const b2 = s[pos.* + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            pos.* += 3;
            return ((@as(u32, b0) & 0x0F) << 12) | ((@as(u32, b1) & 0x3F) << 6) | (@as(u32, b2) & 0x3F);
        }
    } else if ((b0 & 0xF8) == 0xF0 && pos.* + 3 < s.len) {
        const b1 = s[pos.* + 1];
        const b2 = s[pos.* + 2];
        const b3 = s[pos.* + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            pos.* += 4;
            return ((@as(u32, b0) & 0x07) << 18) | ((@as(u32, b1) & 0x3F) << 12) |
                ((@as(u32, b2) & 0x3F) << 6) | (@as(u32, b3) & 0x3F);
        }
    }
    pos.* += 1;
    return 0xFFFD;
}

/// Returns true if `cp` is a zero-width control, format, or combining diacritical mark.
export fn isZeroWidth(cp: u32): bool {
    if (cp == 0 || cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) return true;
    // Combining Diacritical Marks & Extensions
    if (cp >= 0x0300 && cp <= 0x036F) return true;
    if (cp >= 0x1AB0 && cp <= 0x1AFF) return true;
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return true;
    if (cp >= 0x20D0 && cp <= 0x20FF) return true;
    if (cp >= 0xFE20 && cp <= 0xFE2F) return true;
    // Zero-width space / joiners / variation selectors / BOM
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0x2060 || cp == 0xFEFF) return true;
    if (cp >= 0xFE00 && cp <= 0xFE0F) return true;
    if (cp >= 0xE0100 && cp <= 0xE01EF) return true;
    return false;
}

/// Returns true if `cp` occupies 2 terminal columns (East Asian Wide / Fullwidth / Emoji).
export fn isWideChar(cp: u32): bool {
    if (cp < 0x1100) return false;
    // Hangul Jamo
    if (cp <= 0x115F) return true;
    // Angle brackets
    if (cp == 0x2329 || cp == 0x232A) return true;
    // CJK Radicals, Kangxi, Ideographic, Punctuation, Hiragana, Katakana, Bopomofo, Compatibility, Unified Ideographs
    if (cp >= 0x2E80 && cp <= 0xA4CF && cp != 0x303F) return true;
    // Hangul Syllables
    if (cp >= 0xAC00 && cp <= 0xD7A3) return true;
    // CJK Compatibility Ideographs
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;
    // Vertical Forms & CJK Compatibility Forms
    if (cp >= 0xFE10 && cp <= 0xFE19) return true;
    if (cp >= 0xFE30 && cp <= 0xFE6F) return true;
    // Fullwidth ASCII & Halfwidth/Fullwidth Forms
    if (cp >= 0xFF00 && cp <= 0xFF60) return true;
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return true;
    // Emoji & Pictographs (SMP)
    if (cp >= 0x1F300 && cp <= 0x1FAFF) return true;
    // CJK Unified Ideographs Extensions B..G (SIP/TIP)
    if (cp >= 0x20000 && cp <= 0x3FFFD) return true;
    return false;
}

/// Terminal display column width of a single Unicode codepoint (`0`, `1`, or `2`).
export fn codepointWidth(cp: u32): u8 {
    if (isZeroWidth(cp)) return 0;
    if (isWideChar(cp)) return 2;
    return 1;
}

/// Terminal display column width of a UTF-8 string (ignores zero-width marks, counts CJK/Emoji as 2).
export fn strDisplayWidth(s: []const u8): usize {
    let pos: usize = 0;
    let total: usize = 0;
    while (cp := nextCodepoint(s, &pos)) {
        total += @as(usize, codepointWidth(cp));
    }
    return total;
}

/// Return the longest valid UTF-8 byte prefix of `s` whose display width is `<= max_cols`.
export fn sliceToDisplayWidth(s: []const u8, max_cols: usize): []const u8 {
    let pos: usize = 0;
    let cols: usize = 0;
    while (pos < s.len) {
        const before = pos;
        const cp = nextCodepoint(s, &pos) ?? break;
        const w: usize = @as(usize, codepointWidth(cp));
        if (cols + w > max_cols) {
            return s[0..before];
        }
        cols += w;
    }
    return s;
}
