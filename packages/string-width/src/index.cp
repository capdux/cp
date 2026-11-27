// @cap/string-width — Zero-allocation terminal visual column width calculator for CP
// Clean-room MIT implementation with parity to npm `string-width` v7.
// Composes `@cap/ansi-strip` (CSI/SGR/OSC escape stripping) and `@cap/unicode-width`
// (Unicode 15.1 East Asian Width & CJK ambiguous width) with zero duplication.

import {
    hasAnsi,
    stripAnsi,
} from "../../strip-ansi/src/index.cp";
import {
    charWidth,
    charWidthCjk,
} from "../../unicode-width/src/index.cp";

const SW_STRIP_CAP: usize = 4096;

/// Configuration options for `stringWidthWithOptions`.
pub struct StringWidthOptions {
    pub ambiguous_is_narrow: bool,
    pub count_ansi_escape_codes: bool,

    /// Return default `string-width` v7 options:
    /// `ambiguous_is_narrow = true`, `count_ansi_escape_codes = false`.
    pub static fn defaults(): StringWidthOptions {
        return StringWidthOptions {
            ambiguous_is_narrow: true,
            count_ansi_escape_codes: false,
        };
    }
}

struct SwDecoded {
    cp: u32,
    next_pos: usize,
}

fn swDecodeUtf8(s: string, pos: usize): SwDecoded {
    const b0 = s[pos];
    if (b0 < (0x80 as u8)) {
        return SwDecoded { cp: b0 as u32, next_pos: pos + 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8)
        && pos + 1 < s.len
        && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5))
            | ((s[pos + 1] & (0x3F as u8)) as u32);
        return SwDecoded { cp: cp, next_pos: pos + 2 };
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8)
        && pos + 2 < s.len
        && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8)
        && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
            | (((s[pos + 1] & (0x3F as u8)) as u32) << (6 as u5))
            | ((s[pos + 2] & (0x3F as u8)) as u32);
        return SwDecoded { cp: cp, next_pos: pos + 3 };
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8)
        && pos + 3 < s.len
        && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8)
        && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8)
        && (s[pos + 3] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
            | (((s[pos + 1] & (0x3F as u8)) as u32) << (12 as u5))
            | (((s[pos + 2] & (0x3F as u8)) as u32) << (6 as u5))
            | ((s[pos + 3] & (0x3F as u8)) as u32);
        return SwDecoded { cp: cp, next_pos: pos + 4 };
    }
    return SwDecoded { cp: 0xFFFD as u32, next_pos: pos + 1 };
}

fn swIsRegionalIndicator(cp: u32): bool {
    return cp >= (0x1F1E6 as u32) && cp <= (0x1F1FF as u32);
}

fn swIsSkinToneModifier(cp: u32): bool {
    return cp >= (0x1F3FB as u32) && cp <= (0x1F3FF as u32);
}

fn swIsTagChar(cp: u32): bool {
    return cp >= (0xE0020 as u32) && cp <= (0xE007F as u32);
}

fn swBaseCharWidth(cp: u32, ambiguous_is_narrow: bool): usize {
    if (ambiguous_is_narrow) {
        return charWidth(cp);
    }
    return charWidthCjk(cp);
}

/// Measure visual column width of a plain (already ANSI-stripped or raw) UTF-8 string,
/// delegating per-codepoint width to `@cap/unicode-width` while handling emoji ZWJ sequences,
/// Fitzpatrick skin-tone modifiers, Variation Selectors U+FE0F/U+FE0E, and Regional Indicator flag pairs.
fn swMeasurePlain(s: string, ambiguous_is_narrow: bool): usize {
    let total: usize = 0;
    let pos: usize = 0;
    let after_zwj = false;
    let last_base_width: usize = 0;
    let ri_pending = false;

    while (pos < s.len) {
        const decoded = swDecodeUtf8(s, pos);
        const cp = decoded.cp;
        pos = decoded.next_pos;

        // Zero-Width Joiner (U+200D): join following emoji into the preceding grapheme cluster
        if (cp == (0x200D as u32)) {
            after_zwj = true;
            ri_pending = false;
            continue;
        }

        // Variation Selector-16 (U+FE0F): emoji presentation promotes 1-col base to 2-col
        if (cp == (0xFE0F as u32)) {
            if (!after_zwj && last_base_width == 1) {
                total = total + 1;
                last_base_width = 2;
            }
            continue;
        }

        // Variation Selector-15 (U+FE0E): text presentation narrows 2-col emoji to 1-col
        if (cp == (0xFE0E as u32)) {
            if (!after_zwj && last_base_width == 2 && total > 0) {
                total = total - 1;
                last_base_width = 1;
            }
            continue;
        }

        // Fitzpatrick skin-tone modifiers attach to the preceding emoji with 0 extra width
        if (swIsSkinToneModifier(cp)) {
            if (last_base_width > 0) {
                continue;
            }
        }

        // Emoji tag sequences (e.g. subdivision flags) occupy 0 additional columns
        if (swIsTagChar(cp)) {
            continue;
        }

        // Regional Indicator flag pairs (U+1F1E6..U+1F1FF): each pair renders as 2 columns
        if (swIsRegionalIndicator(cp)) {
            after_zwj = false;
            if (ri_pending) {
                // Second RI completes a 2-column flag (first RI already contributed 1 col -> +1 = 2)
                total = total + 1;
                last_base_width = 2;
                ri_pending = false;
            } else {
                total = total + 1;
                last_base_width = 1;
                ri_pending = true;
            }
            continue;
        }
        ri_pending = false;

        const w = swBaseCharWidth(cp, ambiguous_is_narrow);
        if (w == 0) {
            // Combining marks, control characters, zero-width spaces do not reset base width
            continue;
        }

        if (after_zwj) {
            // Joined emoji/character merges into previous cluster without adding columns
            after_zwj = false;
            continue;
        }

        total = total + w;
        last_base_width = w;
    }

    return total;
}

fn swMeasureStrippedChunk(chunk: string, ambiguous_is_narrow: bool): usize {
    if (chunk.len == 0) {
        return 0;
    }
    if (!hasAnsi(chunk)) {
        return swMeasurePlain(chunk, ambiguous_is_narrow);
    }
    if (chunk.len <= SW_STRIP_CAP) {
        let stripped: [4096]u8 = undefined;
        const n = stripAnsi(chunk, stripped[0..SW_STRIP_CAP]);
        return swMeasurePlain(stripped[0..n], ambiguous_is_narrow);
    }
    // Fallback for unusually large inputs > 4096 bytes: split at safe boundaries
    let total: usize = 0;
    let offset: usize = 0;
    while (offset < chunk.len) {
        let end = offset + SW_STRIP_CAP;
        if (end > chunk.len) {
            end = chunk.len;
        } else {
            // Avoid splitting in the middle of a UTF-8 sequence or ANSI ESC sequence
            while (end > offset && (chunk[end] & (0xC0 as u8)) == (0x80 as u8)) {
                end = end - 1;
            }
            if (end == offset) {
                end = offset + SW_STRIP_CAP;
                if (end > chunk.len) {
                    end = chunk.len;
                }
            }
        }
        let stripped: [4096]u8 = undefined;
        const sub = chunk[offset..end];
        const n = stripAnsi(sub, stripped[0..SW_STRIP_CAP]);
        total = total + swMeasurePlain(stripped[0..n], ambiguous_is_narrow);
        offset = end;
    }
    return total;
}

/// Compute visual terminal column width of `input` with configurable `StringWidthOptions`.
pub fn stringWidthWithOptions(input: string, opts: StringWidthOptions): usize {
    if (input.len == 0) {
        return 0;
    }
    if (opts.count_ansi_escape_codes) {
        return swMeasurePlain(input, opts.ambiguous_is_narrow);
    }
    return swMeasureStrippedChunk(input, opts.ambiguous_is_narrow);
}

/// Compute visual terminal column width of `input` (ignoring ANSI CSI/SGR/OSC escape codes,
/// treating East Asian Ambiguous characters as narrow = 1 column).
pub fn stringWidth(input: string): usize {
    return stringWidthWithOptions(input, StringWidthOptions.defaults());
}

/// Compute maximum visual column width across all lines in `multiline`
/// (splitting on `\n` and trimming trailing `\r`).
pub fn maxLineWidthWithOptions(multiline: string, opts: StringWidthOptions): usize {
    if (multiline.len == 0) {
        return 0;
    }
    let max_w: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= multiline.len) {
        if (i == multiline.len || multiline[i] == 10) {
            let end = i;
            if (end > start && multiline[end - 1] == 13) {
                end = end - 1;
            }
            const w = stringWidthWithOptions(multiline[start..end], opts);
            if (w > max_w) {
                max_w = w;
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return max_w;
}

/// Compute maximum visual column width across all lines in `multiline` using default options.
pub fn maxLineWidth(multiline: string): usize {
    return maxLineWidthWithOptions(multiline, StringWidthOptions.defaults());
}

fn swCopyBytes(dst: []u8, pos: usize, src: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < src.len && w < dst.len) {
        dst[w] = src[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn swFillBytes(dst: []u8, pos: usize, ch: u8, count: usize): usize {
    let w = pos;
    let i: usize = 0;
    while (i < count && w < dst.len) {
        dst[w] = ch;
        w = w + 1;
        i = i + 1;
    }
    return w;
}

/// Right-pad `input` with `fill_byte` (defaults to space `' '` if `0`) until its visual
/// terminal column width reaches `target_cols`. Preserves ANSI sequences and UTF-8.
/// Returns total bytes written into `out`.
pub fn padEndVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize {
    const ch: u8 = if (fill_byte == 0) 32 else fill_byte;
    const vis = stringWidth(input);
    let pos = swCopyBytes(out, 0, input);
    if (vis < target_cols) {
        pos = swFillBytes(out, pos, ch, target_cols - vis);
    }
    return pos;
}

/// Left-pad `input` with `fill_byte` (defaults to space `' '` if `0`) until its visual
/// terminal column width reaches `target_cols`. Preserves ANSI sequences and UTF-8.
/// Returns total bytes written into `out`.
pub fn padStartVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize {
    const ch: u8 = if (fill_byte == 0) 32 else fill_byte;
    const vis = stringWidth(input);
    let pos: usize = 0;
    if (vis < target_cols) {
        pos = swFillBytes(out, pos, ch, target_cols - vis);
    }
    pos = swCopyBytes(out, pos, input);
    return pos;
}

export {
    StringWidthOptions,
    stringWidth,
    stringWidthWithOptions,
    maxLineWidth,
    maxLineWidthWithOptions,
    padEndVisible,
    padStartVisible,
};
