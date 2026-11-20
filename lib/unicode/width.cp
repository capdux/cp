// Source: bun/src/unicode/uucode_lib/src/x/config_x/wcwidth.cp -> cp/lib/unicode/width.cp (East Asian Width, terminal display width & safe truncation)

const std = @import("std");
import { UnicodeError, decodeUtf8Lossy } from "./utf.cp";
import { GraphemeIterator } from "./grapheme.cp";

fn isZeroWidthCodepoint(cp: u32): bool {
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) {
        return true;
    }
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE20 && cp <= 0xFE2F))
    {
        return true;
    }
    if ((cp >= 0x200B && cp <= 0x200F) ||
        (cp >= 0x202A && cp <= 0x202E) ||
        (cp >= 0x2060 && cp <= 0x206F) ||
        cp == 0xFEFF)
    {
        return true;
    }
    if ((cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xE0100 && cp <= 0xE01EF)) {
        return true;
    }
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) {
        return true;
    }
    if (cp >= 0x1160 && cp <= 0x11FF) {
        return true;
    }
    return false;
}

fn isWideCodepoint(cp: u32): bool {
    if (cp >= 0x1100 && cp <= 0x115F) {
        return true;
    }
    if (cp == 0x231A || cp == 0x231B || cp == 0x2329 || cp == 0x232A || (cp >= 0x23E9 && cp <= 0x23F3)) {
        return true;
    }
    if ((cp >= 0x2600 && cp <= 0x27BF) || (cp >= 0x2B50 && cp <= 0x2B55)) {
        return true;
    }
    if ((cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3040 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF01 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6))
    {
        return true;
    }
    if ((cp >= 0x1F1E6 && cp <= 0x1F1FF) ||
        (cp >= 0x1F300 && cp <= 0x1F64F) ||
        (cp >= 0x1F680 && cp <= 0x1F6FF) ||
        (cp >= 0x1F900 && cp <= 0x1F9FF) ||
        (cp >= 0x1FA70 && cp <= 0x1FAFF))
    {
        return true;
    }
    if ((cp >= 0x20000 && cp <= 0x2FFFD) || (cp >= 0x30000 && cp <= 0x3FFFD)) {
        return true;
    }
    return false;
}

/// Returns the terminal monospace column width (`0`, `1`, or `2`) of a Unicode codepoint.
pub fn codepointWidth(cp: u32): u8 {
    if (cp == 0x00AD) {
        return 1;
    }
    if (cp >= 0x20 && cp <= 0x7E) {
        return 1;
    }
    if (isZeroWidthCodepoint(cp)) {
        return 0;
    }
    if (isWideCodepoint(cp)) {
        return 2;
    }
    return 1;
}

fn clusterWidth(cluster: []const u8): usize {
    if (cluster.len == 0) {
        return 0;
    }
    const decoded = decodeUtf8Lossy(cluster, 0);
    return codepointWidth(decoded.codepoint) as usize;
}

/// Computes the terminal monospace display width of UTF-8 string `s` (handles grapheme clusters & ANSI escapes).
pub fn displayWidth(s: []const u8): usize {
    let width: usize = 0;
    let i: usize = 0;

    while (i < s.len) {
        const b = s[i];
        if (b == 0x1B && i + 1 < s.len) {
            const next_b = s[i + 1];
            if (next_b == '[') {
                i += 2;
                while (i < s.len) {
                    const c = s[i];
                    i += 1;
                    if (c >= 0x40 && c <= 0x7E) {
                        break;
                    }
                }
                continue;
            }
            if (next_b == ']') {
                i += 2;
                while (i < s.len) {
                    if (s[i] == 0x07) {
                        i += 1;
                        break;
                    }
                    if (s[i] == 0x1B && i + 1 < s.len && s[i + 1] == '\\') {
                        i += 2;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
        }

        // Fast ASCII run until non-ASCII or ESC
        if (b >= 0x20 && b <= 0x7E) {
            const run_start = i;
            while (i < s.len && s[i] >= 0x20 && s[i] <= 0x7E) {
                i += 1;
            }
            // If next byte starts a combining mark or ZWJ, step back 1 byte so GraphemeIterator handles it
            if (i < s.len && s[i] >= 0x80) {
                if (i - 1 > run_start) {
                    width += (i - 1) - run_start;
                }
                i -= 1;
            } else {
                width += i - run_start;
                continue;
            }
        }

        // Find next ESC to bound grapheme iteration segment
        let seg_end = i;
        while (seg_end < s.len && s[seg_end] != 0x1B) {
            seg_end += 1;
        }
        let it = GraphemeIterator.init(s[i..seg_end]);
        while (cluster := it.next()) {
            width += clusterWidth(cluster);
        }
        i = seg_end;
    }

    return width;
}

/// Safely truncates `s` to fit within `max_width` terminal columns without splitting UTF-8 or grapheme clusters.
/// Appends `ellipsis` when truncation occurs; returns a newly allocated UTF-8 slice.
pub fn truncateByWidth(
    alloc: std.mem.Allocator,
    s: []const u8,
    max_width: usize,
    ellipsis: []const u8,
): ([]u8) ! UnicodeError {
    const total_w = displayWidth(s);
    if (total_w <= max_width) {
        let copy = alloc.alloc(u8, s.len) catch return UnicodeError.OutOfMemory;
        if (s.len > 0) {
            @memcpy(copy, s);
        }
        return copy;
    }

    if (max_width == 0) {
        return alloc.alloc(u8, 0) catch return UnicodeError.OutOfMemory;
    }

    const ellipsis_w = displayWidth(ellipsis);
    if (ellipsis_w > max_width) {
        return truncateByWidth(alloc, ellipsis, max_width, "");
    }

    const budget = max_width - ellipsis_w;
    let it = GraphemeIterator.init(s);
    let cut_end: usize = 0;
    let used_w: usize = 0;

    while (span := it.nextSpan()) {
        const cw = clusterWidth(s[span.start..span.end]);
        if (used_w + cw > budget) {
            break;
        }
        used_w += cw;
        cut_end = span.end;
    }

    let out = alloc.alloc(u8, cut_end + ellipsis.len) catch return UnicodeError.OutOfMemory;
    if (cut_end > 0) {
        @memcpy(out[0..cut_end], s[0..cut_end]);
    }
    if (ellipsis.len > 0) {
        @memcpy(out[cut_end..], ellipsis);
    }
    return out;
}
