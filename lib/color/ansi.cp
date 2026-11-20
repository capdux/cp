// Source: cp/lib/color/ansi.cp (ANSI TrueColor, xterm-256 Quantization & Strip)
// Pure CP terminal ANSI 24-bit TrueColor SGR encoder, xterm-256 Euclidean quantizer,
// and CSI/OSC/ESC ANSI escape sequence stripper.

const std = @import("std");
import { ColorError, Rgba } from "./spaces.cp";

const CUBE_LEVELS: [6]u8 = [0, 95, 135, 175, 215, 255];
const ASCII_ZERO: u8 = '0';

fn appendU8Decimal(buf: *[32]u8, pos: *usize, val: u8) {
    if (val >= 100) {
        buf[pos.*] = ASCII_ZERO + (val / 100);
        pos.* += 1;
        buf[pos.*] = ASCII_ZERO + ((val / 10) % 10);
        pos.* += 1;
        buf[pos.*] = ASCII_ZERO + (val % 10);
        pos.* += 1;
    } else if (val >= 10) {
        buf[pos.*] = ASCII_ZERO + (val / 10);
        pos.* += 1;
        buf[pos.*] = ASCII_ZERO + (val % 10);
        pos.* += 1;
    } else {
        buf[pos.*] = ASCII_ZERO + val;
        pos.* += 1;
    }
}

fn encodeTrueColorSgr(alloc: std.mem.Allocator, prefix_code: u8, c: Rgba): ([]u8) ! ColorError {
    let tmp: [32]u8 = undefined;
    let pos: usize = 0;

    tmp[pos] = 0x1B;
    pos += 1;
    tmp[pos] = '[';
    pos += 1;
    tmp[pos] = prefix_code; // '3' for fg (38), '4' for bg (48)
    pos += 1;
    tmp[pos] = '8';
    pos += 1;
    tmp[pos] = ';';
    pos += 1;
    tmp[pos] = '2';
    pos += 1;
    tmp[pos] = ';';
    pos += 1;
    appendU8Decimal(&tmp, &pos, c.r);
    tmp[pos] = ';';
    pos += 1;
    appendU8Decimal(&tmp, &pos, c.g);
    tmp[pos] = ';';
    pos += 1;
    appendU8Decimal(&tmp, &pos, c.b);
    tmp[pos] = 'm';
    pos += 1;

    return alloc.dupe(u8, tmp[0..pos]) catch return ColorError.OutOfMemory;
}

fn nearestCubeIndex(v: u8): usize {
    let best_idx: usize = 0;
    let best_diff: i32 = 1000000;
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        const diff = @as(i32, v) - @as(i32, CUBE_LEVELS[i]);
        const adiff = if (diff < 0) -diff else diff;
        if (adiff < best_diff) {
            best_diff = adiff;
            best_idx = i;
        }
    }
    return best_idx;
}

fn rgbDistSq(r1: u8, g1: u8, b1: u8, r2: u8, g2: u8, b2: u8): i32 {
    const dr = @as(i32, r1) - @as(i32, r2);
    const dg = @as(i32, g1) - @as(i32, g2);
    const db = @as(i32, b1) - @as(i32, b2);
    return dr * dr + dg * dg + db * db;
}

/// Returns the end index of an ANSI escape sequence starting at `text[start] == 0x1B`.
fn escapeSeqEnd(text: string, start: usize): usize {
    const n = text.len;
    if (start + 1 >= n) return n;

    const next = text[start + 1];
    // CSI sequence: ESC [ <params 0x30..0x3F>* <intermediates 0x20..0x2F>* <final 0x40..0x7E>
    if (next == '[') {
        let j = start + 2;
        while (j < n && text[j] >= 0x20 && text[j] <= 0x3F) : (j += 1) {}
        if (j < n && text[j] >= 0x40 && text[j] <= 0x7E) {
            j += 1;
        }
        return j;
    }

    // OSC sequence: ESC ] ... (BEL 0x07 | ST ESC \)
    if (next == ']') {
        let j = start + 2;
        while (j < n) {
            if (text[j] == 0x07) {
                return j + 1;
            }
            if (text[j] == 0x1B && j + 1 < n && text[j + 1] == '\\') {
                return j + 2;
            }
            j += 1;
        }
        return n;
    }

    // Standard 2-byte ESC Fe/Fp/Fs sequence (e.g. ESC c, ESC M, ESC =, ESC >)
    if (next >= 0x20 && next <= 0x7E) {
        return start + 2;
    }
    return start + 1;
}

/// Terminal ANSI TrueColor SGR encoder, xterm-256 quantizer, and escape sequence stripper.
pub static struct AnsiColorCodec {
    /// Encodes a 24-bit foreground TrueColor SGR sequence `\x1b[38;2;r;g;bm`.
    static fn fgTrueColor(alloc: std.mem.Allocator, c: Rgba): ([]u8) ! ColorError {
        return encodeTrueColorSgr(alloc, '3', c);
    }

    /// Encodes a 24-bit background TrueColor SGR sequence `\x1b[48;2;r;g;bm`.
    static fn bgTrueColor(alloc: std.mem.Allocator, c: Rgba): ([]u8) ! ColorError {
        return encodeTrueColorSgr(alloc, '4', c);
    }

    /// Maps an sRGB color to the closest xterm-256 palette index (`16..255`) by Euclidean RGB distance
    /// across the 6x6x6 color cube (`16..231`) and 24-step grayscale ramp (`232..255`).
    static fn rgbToAnsi256(c: Rgba): u8 {
        const ri = nearestCubeIndex(c.r);
        const gi = nearestCubeIndex(c.g);
        const bi = nearestCubeIndex(c.b);

        const cube_r = CUBE_LEVELS[ri];
        const cube_g = CUBE_LEVELS[gi];
        const cube_b = CUBE_LEVELS[bi];
        const cube_dist = rgbDistSq(c.r, c.g, c.b, cube_r, cube_g, cube_b);
        const cube_code: u8 = (16 + 36 * ri + 6 * gi + bi) as u8;

        let best_gray_idx: usize = 0;
        let best_gray_dist: i32 = 1000000000;
        let g_idx: usize = 0;
        while (g_idx < 24) : (g_idx += 1) {
            const gv: u8 = (8 + 10 * g_idx) as u8;
            const d = rgbDistSq(c.r, c.g, c.b, gv, gv, gv);
            if (d < best_gray_dist) {
                best_gray_dist = d;
                best_gray_idx = g_idx;
            }
        }

        if (best_gray_dist < cube_dist) {
            return (232 + best_gray_idx) as u8;
        }
        return cube_code;
    }

    /// Strips all ANSI CSI/SGR, OSC, and 2-byte ESC sequences from `text`, preserving UTF-8 payloads.
    static fn stripAnsi(alloc: std.mem.Allocator, text: string): ([]u8) ! ColorError {
        let clean_len: usize = 0;
        let i: usize = 0;
        while (i < text.len) {
            if (text[i] == 0x1B) {
                i = escapeSeqEnd(text, i);
            } else {
                clean_len += 1;
                i += 1;
            }
        }

        let out = alloc.alloc(u8, clean_len) catch return ColorError.OutOfMemory;
        let out_pos: usize = 0;
        let j: usize = 0;
        while (j < text.len) {
            if (text[j] == 0x1B) {
                j = escapeSeqEnd(text, j);
            } else {
                out[out_pos] = text[j];
                out_pos += 1;
                j += 1;
            }
        }
        return out;
    }
}
