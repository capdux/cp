// Source: cp/lib/table/width.cp (ANSI & East-Asian Wide Character DisplayWidth Engine)
// Pure CP, zero-dependency display width calculation, ANSI-safe truncation,
// word/character-boundary wrapping, and alignment padding.

const std = @import("std");

pub enum TextAlign {
    Left,
    Center,
    Right,
}

pub err TableError {
    ColumnMismatch,
    InvalidWidth,
    OutOfMemory,
}

struct Utf8Token {
    cp: u32,
    byte_len: usize,
    width: usize,
}

pub struct DisplayWidth {
    /// Returns true if byte `b` is a CSI parameter/intermediate byte (0x20..=0x3F).
    static fn isCsiParamByte(b: u8): bool {
        return b >= 0x20 && b <= 0x3F;
    }

    /// Returns true if byte `b` is a CSI final byte (0x40..=0x7E).
    static fn isCsiFinalByte(b: u8): bool {
        return b >= 0x40 && b <= 0x7E;
    }

    /// If `s[idx..]` starts with an ANSI CSI sequence (`\x1b[...<final>`),
    /// returns the total byte length of that escape sequence; otherwise 0.
    pub static fn ansiSeqLen(s: string, idx: usize): usize {
        if (idx + 1 >= s.len) {
            return 0;
        }
        if (s[idx] != 0x1B || s[idx + 1] != '[') {
            return 0;
        }
        let j: usize = idx + 2;
        while (j < s.len) : (j += 1) {
            const b = s[j];
            if (DisplayWidth.isCsiFinalByte(b)) {
                return (j - idx) + 1;
            }
            if (!DisplayWidth.isCsiParamByte(b)) {
                return 2;
            }
        }
        return s.len - idx;
    }

    /// Decodes a single UTF-8 codepoint at `s[idx..]`.
    static fn decodeUtf8At(s: string, idx: usize): Utf8Token {
        const b0 = s[idx];
        if (b0 < 0x80) {
            const w = DisplayWidth.charWidth(u32(b0));
            return Utf8Token{ cp: u32(b0), byte_len: 1, width: w };
        }

        let seq_len: usize = 1;
        let cp: u32 = 0xFFFD;
        if ((b0 & 0xE0) == 0xC0 && idx + 1 < s.len) {
            const b1 = s[idx + 1];
            if ((b1 & 0xC0) == 0x80) {
                cp = (u32(b0 & 0x1F) << 6) | u32(b1 & 0x3F);
                if (cp >= 0x80) {
                    seq_len = 2;
                } else {
                    cp = 0xFFFD;
                }
            }
        } else if ((b0 & 0xF0) == 0xE0 && idx + 2 < s.len) {
            const b1 = s[idx + 1];
            const b2 = s[idx + 2];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
                cp = (u32(b0 & 0x0F) << 12) | (u32(b1 & 0x3F) << 6) | u32(b2 & 0x3F);
                if (cp >= 0x800 && !(cp >= 0xD800 && cp <= 0xDFFF)) {
                    seq_len = 3;
                } else {
                    cp = 0xFFFD;
                }
            }
        } else if ((b0 & 0xF8) == 0xF0 && idx + 3 < s.len) {
            const b1 = s[idx + 1];
            const b2 = s[idx + 2];
            const b3 = s[idx + 3];
            if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
                cp = (u32(b0 & 0x07) << 18) | (u32(b1 & 0x3F) << 12) | (u32(b2 & 0x3F) << 6) | u32(b3 & 0x3F);
                if (cp >= 0x10000 && cp <= 0x10FFFF) {
                    seq_len = 4;
                } else {
                    cp = 0xFFFD;
                }
            }
        }

        return Utf8Token{
            cp: cp,
            byte_len: seq_len,
            width: DisplayWidth.charWidth(cp),
        };
    }

    /// Returns terminal column width (0, 1, or 2) for a Unicode codepoint.
    pub static fn charWidth(cp: u32): usize {
        if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) {
            return 0;
        }
        // Combining marks, zero-width characters, variation selectors
        if ((cp >= 0x0300 && cp <= 0x036F) ||
            (cp >= 0x1AB0 && cp <= 0x1AFF) ||
            (cp >= 0x1DC0 && cp <= 0x1DFF) ||
            (cp >= 0x200B && cp <= 0x200F) ||
            (cp >= 0x202A && cp <= 0x202E) ||
            (cp >= 0x2060 && cp <= 0x206F) ||
            (cp >= 0x20D0 && cp <= 0x20FF) ||
            (cp >= 0xFE00 && cp <= 0xFE0F) ||
            (cp >= 0xFE20 && cp <= 0xFE2F) ||
            cp == 0xFEFF ||
            (cp >= 0xE0100 && cp <= 0xE01EF))
        {
            return 0;
        }
        // East Asian Wide, Fullwidth & Emoji
        if ((cp >= 0x1100 && cp <= 0x115F) ||
            cp == 0x231A || cp == 0x231B ||
            cp == 0x2329 || cp == 0x232A ||
            (cp >= 0x23E9 && cp <= 0x23F3) ||
            (cp >= 0x23F8 && cp <= 0x23FA) ||
            (cp >= 0x25FD && cp <= 0x25FE) ||
            cp == 0x2614 || cp == 0x2615 ||
            (cp >= 0x2648 && cp <= 0x2653) ||
            cp == 0x267F || cp == 0x2693 || cp == 0x26A1 ||
            cp == 0x26AA || cp == 0x26AB ||
            cp == 0x26BD || cp == 0x26BE ||
            cp == 0x26C4 || cp == 0x26C5 ||
            cp == 0x26CE || cp == 0x26D4 || cp == 0x26EA ||
            cp == 0x26F2 || cp == 0x26F3 || cp == 0x26F5 ||
            cp == 0x26FA || cp == 0x26FD ||
            cp == 0x2705 || cp == 0x270A || cp == 0x270B ||
            cp == 0x2728 || cp == 0x274C || cp == 0x274E ||
            (cp >= 0x2753 && cp <= 0x2755) || cp == 0x2757 ||
            (cp >= 0x2795 && cp <= 0x2797) ||
            cp == 0x27B0 || cp == 0x27BF ||
            cp == 0x2B1B || cp == 0x2B1C ||
            cp == 0x2B50 || cp == 0x2B55 ||
            (cp >= 0x2E80 && cp <= 0x2EFF) ||
            (cp >= 0x2F00 && cp <= 0x2FDF) ||
            (cp >= 0x2FF0 && cp <= 0x303E) ||
            (cp >= 0x3041 && cp <= 0x33FF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0xA000 && cp <= 0xA4CF) ||
            (cp >= 0xA960 && cp <= 0xA97F) ||
            (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFE10 && cp <= 0xFE19) ||
            (cp >= 0xFE30 && cp <= 0xFE6F) ||
            (cp >= 0xFF01 && cp <= 0xFF60) ||
            (cp >= 0xFFE0 && cp <= 0xFFE6) ||
            (cp >= 0x1F300 && cp <= 0x1F64F) ||
            (cp >= 0x1F680 && cp <= 0x1F6FF) ||
            (cp >= 0x1F900 && cp <= 0x1F9FF) ||
            (cp >= 0x1FA00 && cp <= 0x1FAFF) ||
            (cp >= 0x20000 && cp <= 0x2FFFD) ||
            (cp >= 0x30000 && cp <= 0x3FFFD))
        {
            return 2;
        }
        return 1;
    }

    /// Computes visible terminal column width of `s`, ignoring ANSI CSI sequences.
    pub static fn visibleWidth(s: string): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            const seq = DisplayWidth.ansiSeqLen(s, i);
            if (seq > 0) {
                i += seq;
                continue;
            }
            const tok = DisplayWidth.decodeUtf8At(s, i);
            total += tok.width;
            i += tok.byte_len;
        }
        return total;
    }

    /// Strips all ANSI CSI sequences from `s`, returning an owned plain string.
    pub static fn stripAnsi(alloc: std.mem.Allocator, s: string): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < s.len) {
            const seq = DisplayWidth.ansiSeqLen(s, i);
            if (seq > 0) {
                i += seq;
                continue;
            }
            out.append(alloc, s[i])?;
            i += 1;
        }
        return out.toOwnedSlice(alloc);
    }

    /// Truncates `s` to at most `max_width` visible columns, appending `ellipsis`
    /// when truncated and preserving a trailing `\x1b[0m` reset if ANSI codes were emitted.
    /// Never splits a multi-byte UTF-8 character.
    pub static fn truncate(
        alloc: std.mem.Allocator,
        s: string,
        max_width: usize,
        ellipsis: string,
    ): ([]u8)! {
        const total_w = DisplayWidth.visibleWidth(s);
        if (total_w <= max_width) {
            return alloc.dupe(u8, s);
        }
        if (max_width == 0) {
            return alloc.dupe(u8, "");
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const ell_w = DisplayWidth.visibleWidth(ellipsis);
        let budget: usize = max_width;
        let use_ellipsis = false;
        if (ell_w <= max_width) {
            budget = max_width - ell_w;
            use_ellipsis = true;
        }

        let used_w: usize = 0;
        let emitted_ansi = false;
        let i: usize = 0;
        while (i < s.len) {
            const seq = DisplayWidth.ansiSeqLen(s, i);
            if (seq > 0) {
                out.appendSlice(alloc, s[i..(i + seq)])?;
                emitted_ansi = true;
                i += seq;
                continue;
            }
            const tok = DisplayWidth.decodeUtf8At(s, i);
            if (used_w + tok.width > budget) {
                break;
            }
            out.appendSlice(alloc, s[i..(i + tok.byte_len)])?;
            used_w += tok.width;
            i += tok.byte_len;
        }

        if (use_ellipsis && ellipsis.len > 0) {
            out.appendSlice(alloc, ellipsis)?;
        }

        if (emitted_ansi) {
            const reset = "\x1b[0m";
            if (!std.mem.endsWith(u8, out.items, reset)) {
                out.appendSlice(alloc, reset)?;
            }
        }

        return out.toOwnedSlice(alloc);
    }

    /// Wraps `s` into lines each with visible width <= `max_width`,
    /// splitting on `\n` first and then on word/character boundaries.
    pub static fn wrap(alloc: std.mem.Allocator, s: string, max_width: usize): ([][]u8)! {
        if (max_width == 0) {
            return TableError.InvalidWidth;
        }

        let lines: std.ArrayList([]u8) = .empty;
        errdefer {
            for (line of lines.items) {
                alloc.free(line);
            }
            lines.deinit(alloc);
        }

        let line_start: usize = 0;
        let pos: usize = 0;
        while (pos <= s.len) {
            if (pos == s.len || s[pos] == '\n') {
                let end = pos;
                if (end > line_start && s[end - 1] == '\r') {
                    end -= 1;
                }
                const segment = s[line_start..end];
                DisplayWidth.wrapSingleLine(alloc, &lines, segment, max_width)?;
                line_start = pos + 1;
            }
            pos += 1;
        }

        return lines.toOwnedSlice(alloc);
    }

    static fn wrapSingleLine(
        alloc: std.mem.Allocator,
        lines: *std.ArrayList([]u8),
        segment: string,
        max_width: usize,
    ): ! {
        if (DisplayWidth.visibleWidth(segment) <= max_width) {
            const dup = alloc.dupe(u8, segment)?;
            errdefer alloc.free(dup);
            lines.append(alloc, dup)?;
            return;
        }

        let cur: std.ArrayList(u8) = .empty;
        defer cur.deinit(alloc);

        let cur_w: usize = 0;
        let last_space_out_len: usize = 0;
        let last_space_src_next: usize = 0;
        let has_space = false;

        let i: usize = 0;
        while (i < segment.len) {
            const seq = DisplayWidth.ansiSeqLen(segment, i);
            if (seq > 0) {
                cur.appendSlice(alloc, segment[i..(i + seq)])?;
                i += seq;
                continue;
            }

            const tok = DisplayWidth.decodeUtf8At(segment, i);
            if (cur_w > 0 && cur_w + tok.width > max_width) {
                if (tok.cp == ' ') {
                    const owned = cur.toOwnedSlice(alloc)?;
                    errdefer alloc.free(owned);
                    lines.append(alloc, owned)?;
                    cur_w = 0;
                    has_space = false;
                    i += tok.byte_len;
                    continue;
                }
                if (has_space) {
                    const line_slice = alloc.dupe(u8, cur.items[0..last_space_out_len])?;
                    errdefer alloc.free(line_slice);
                    lines.append(alloc, line_slice)?;
                    cur.clearRetainingCapacity();
                    cur_w = 0;
                    has_space = false;
                    i = last_space_src_next;
                    continue;
                }
                const owned = cur.toOwnedSlice(alloc)?;
                errdefer alloc.free(owned);
                lines.append(alloc, owned)?;
                cur_w = 0;
                has_space = false;
            }

            if (tok.cp == ' ') {
                last_space_out_len = cur.items.len;
                last_space_src_next = i + tok.byte_len;
                has_space = true;
            }

            cur.appendSlice(alloc, segment[i..(i + tok.byte_len)])?;
            cur_w += tok.width;
            i += tok.byte_len;
        }

        if (cur.items.len > 0 || lines.items.len == 0) {
            const owned = cur.toOwnedSlice(alloc)?;
            errdefer alloc.free(owned);
            lines.append(alloc, owned)?;
        }
    }

    /// Frees a slice of wrapped lines returned by `wrap`.
    pub static fn freeLines(alloc: std.mem.Allocator, lines: [][]u8) {
        for (line of lines) {
            alloc.free(line);
        }
        if (lines.len > 0) {
            alloc.free(lines);
        }
    }

    /// Pads `s` with spaces to `target_width` visible columns according to `alignment`.
    pub static fn pad(
        alloc: std.mem.Allocator,
        s: string,
        target_width: usize,
        alignment: TextAlign,
    ): ([]u8)! {
        const vis = DisplayWidth.visibleWidth(s);
        if (vis >= target_width) {
            return alloc.dupe(u8, s);
        }
        const diff = target_width - vis;
        let left_pad: usize = 0;
        let right_pad: usize = 0;
        switch (alignment) {
            .Left => {
                right_pad = diff;
            },
            .Right => {
                left_pad = diff;
            },
            .Center => {
                left_pad = diff / 2;
                right_pad = diff - left_pad;
            },
        }

        let out = alloc.alloc(u8, left_pad + s.len + right_pad)?;
        let pos: usize = 0;
        let j: usize = 0;
        while (j < left_pad) : (j += 1) {
            out[pos] = ' ';
            pos += 1;
        }
        if (s.len > 0) {
            @memcpy(out[pos..(pos + s.len)], s);
            pos += s.len;
        }
        j = 0;
        while (j < right_pad) : (j += 1) {
            out[pos] = ' ';
            pos += 1;
        }
        return out;
    }
}
