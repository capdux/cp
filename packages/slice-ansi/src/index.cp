// @cap/slice-ansi — Zero-allocation ANSI-preserving terminal column slicer & truncator
// Clean-room MIT implementation in pure CP.
// Reuses @cap/anstyle-parse for VT100/ANSI tokenization & SGR parameter parsing,
// and @cap/unicode-width for East Asian / CJK / Emoji display column width calculation.

import {
    TOKEN_TEXT,
    TOKEN_CSI,
    AnsiToken,
    SgrParams,
    tokenizeAnsi,
    parseCsiParams,
} from "../../anstyle-parse/src/index.cp";
import { charWidth, strWidth } from "../../unicode-width/src/index.cp";

const SA_ESC: u8 = 27;
const SA_COLOR_NONE: u8 = 0;
const SA_COLOR_BASIC: u8 = 1;
const SA_COLOR_256: u8 = 2;
const SA_COLOR_RGB: u8 = 3;
const SA_MAX_END: usize = 0x3FFFFFFF;

struct SaUtf8Char {
    cp: u32,
    byte_len: usize,
}

struct SaStyleState {
    bold: bool,
    dim: bool,
    italic: bool,
    underline: bool,
    blink: bool,
    inverse: bool,
    hidden: bool,
    strikethrough: bool,
    fg_mode: u8,
    fg_code: u16,
    fg_r: u8,
    fg_g: u8,
    fg_b: u8,
    bg_mode: u8,
    bg_code: u16,
    bg_r: u8,
    bg_g: u8,
    bg_b: u8,

    static fn empty(): SaStyleState {
        return SaStyleState {
            bold: false,
            dim: false,
            italic: false,
            underline: false,
            blink: false,
            inverse: false,
            hidden: false,
            strikethrough: false,
            fg_mode: SA_COLOR_NONE,
            fg_code: 0,
            fg_r: 0,
            fg_g: 0,
            fg_b: 0,
            bg_mode: SA_COLOR_NONE,
            bg_code: 0,
            bg_r: 0,
            bg_g: 0,
            bg_b: 0,
        };
    }

    fn reset(*_) {
        _.bold = false;
        _.dim = false;
        _.italic = false;
        _.underline = false;
        _.blink = false;
        _.inverse = false;
        _.hidden = false;
        _.strikethrough = false;
        _.fg_mode = SA_COLOR_NONE;
        _.fg_code = 0;
        _.fg_r = 0;
        _.fg_g = 0;
        _.fg_b = 0;
        _.bg_mode = SA_COLOR_NONE;
        _.bg_code = 0;
        _.bg_r = 0;
        _.bg_g = 0;
        _.bg_b = 0;
    }

    fn hasAttr(): bool {
        return _.bold || _.dim || _.italic || _.underline || _.blink || _.inverse || _.hidden || _.strikethrough;
    }

    fn hasAny(): bool {
        return _.hasAttr() || _.fg_mode != SA_COLOR_NONE || _.bg_mode != SA_COLOR_NONE;
    }

    fn applySgr(*_, seq: string) {
        const parsed: SgrParams = parseCsiParams(seq);
        if (!parsed.valid) {
            return;
        }
        let idx: usize = 0;
        while (idx < parsed.count) {
            const p: u16 = parsed.params[idx];
            if (p == 0) {
                _.reset();
            } else if (p == 1) {
                _.bold = true;
            } else if (p == 2) {
                _.dim = true;
            } else if (p == 3) {
                _.italic = true;
            } else if (p == 4) {
                _.underline = true;
            } else if (p == 5 || p == 6) {
                _.blink = true;
            } else if (p == 7) {
                _.inverse = true;
            } else if (p == 8) {
                _.hidden = true;
            } else if (p == 9) {
                _.strikethrough = true;
            } else if (p == 22) {
                _.bold = false;
                _.dim = false;
            } else if (p == 23) {
                _.italic = false;
            } else if (p == 24) {
                _.underline = false;
            } else if (p == 25) {
                _.blink = false;
            } else if (p == 27) {
                _.inverse = false;
            } else if (p == 28) {
                _.hidden = false;
            } else if (p == 29) {
                _.strikethrough = false;
            } else if ((p >= 30 && p <= 37) || (p >= 90 && p <= 97)) {
                _.fg_mode = SA_COLOR_BASIC;
                _.fg_code = p;
            } else if (p == 38) {
                if (idx + 2 < parsed.count && parsed.params[idx + 1] == 5) {
                    _.fg_mode = SA_COLOR_256;
                    _.fg_code = parsed.params[idx + 2];
                    idx += 2;
                } else if (idx + 4 < parsed.count && parsed.params[idx + 1] == 2) {
                    _.fg_mode = SA_COLOR_RGB;
                    _.fg_r = (parsed.params[idx + 2] & 0xFF) as u8;
                    _.fg_g = (parsed.params[idx + 3] & 0xFF) as u8;
                    _.fg_b = (parsed.params[idx + 4] & 0xFF) as u8;
                    idx += 4;
                }
            } else if (p == 39) {
                _.fg_mode = SA_COLOR_NONE;
            } else if ((p >= 40 && p <= 47) || (p >= 100 && p <= 107)) {
                _.bg_mode = SA_COLOR_BASIC;
                _.bg_code = p;
            } else if (p == 48) {
                if (idx + 2 < parsed.count && parsed.params[idx + 1] == 5) {
                    _.bg_mode = SA_COLOR_256;
                    _.bg_code = parsed.params[idx + 2];
                    idx += 2;
                } else if (idx + 4 < parsed.count && parsed.params[idx + 1] == 2) {
                    _.bg_mode = SA_COLOR_RGB;
                    _.bg_r = (parsed.params[idx + 2] & 0xFF) as u8;
                    _.bg_g = (parsed.params[idx + 3] & 0xFF) as u8;
                    _.bg_b = (parsed.params[idx + 4] & 0xFF) as u8;
                    idx += 4;
                }
            } else if (p == 49) {
                _.bg_mode = SA_COLOR_NONE;
            }
            idx += 1;
        }
    }
}

fn saDecodeUtf8(s: string, pos: usize): SaUtf8Char {
    const b0: u8 = s[pos];
    if (b0 < (0x80 as u8)) {
        return SaUtf8Char { cp: b0 as u32, byte_len: 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((s[pos + 1] & (0x3F as u8)) as u32);
        return SaUtf8Char { cp: cp, byte_len: 2 };
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
            | (((s[pos + 1] & (0x3F as u8)) as u32) << (6 as u5))
            | ((s[pos + 2] & (0x3F as u8)) as u32);
        return SaUtf8Char { cp: cp, byte_len: 3 };
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < s.len && (s[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 2] & (0xC0 as u8)) == (0x80 as u8) && (s[pos + 3] & (0xC0 as u8)) == (0x80 as u8)) {
        const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
            | (((s[pos + 1] & (0x3F as u8)) as u32) << (12 as u5))
            | (((s[pos + 2] & (0x3F as u8)) as u32) << (6 as u5))
            | ((s[pos + 3] & (0x3F as u8)) as u32);
        return SaUtf8Char { cp: cp, byte_len: 4 };
    }
    return SaUtf8Char { cp: 0xFFFD as u32, byte_len: 1 };
}

fn saWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn saWriteBytes(out: []u8, pos: usize, src: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < src.len && w < out.len) {
        out[w] = src[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn saWriteU16(out: []u8, pos: usize, val: u16): usize {
    let digits: [5]u8 = [0, 0, 0, 0, 0];
    let n: usize = 0;
    let v: u16 = val;
    if (v == 0) {
        return saWriteByte(out, pos, 48);
    }
    while (v > 0 && n < 5) {
        digits[n] = (48 + (v % 10)) as u8;
        v = v / 10;
        n += 1;
    }
    let w: usize = pos;
    while (n > 0) {
        n -= 1;
        w = saWriteByte(out, w, digits[n]);
    }
    return w;
}

fn saWriteSgrParam(out: []u8, pos: usize, first: *bool, code: u16): usize {
    let w: usize = pos;
    if (!first.*) {
        w = saWriteByte(out, w, 59); // ';'
    }
    first.* = false;
    return saWriteU16(out, w, code);
}

fn saEmitActiveStyle(out: []u8, pos: usize, st: SaStyleState): usize {
    if (!st.hasAny()) {
        return pos;
    }
    let w: usize = pos;
    w = saWriteByte(out, w, SA_ESC);
    w = saWriteByte(out, w, 91); // '['
    let first: bool = true;

    if (st.bold) { w = saWriteSgrParam(out, w, &first, 1); }
    if (st.dim) { w = saWriteSgrParam(out, w, &first, 2); }
    if (st.italic) { w = saWriteSgrParam(out, w, &first, 3); }
    if (st.underline) { w = saWriteSgrParam(out, w, &first, 4); }
    if (st.blink) { w = saWriteSgrParam(out, w, &first, 5); }
    if (st.inverse) { w = saWriteSgrParam(out, w, &first, 7); }
    if (st.hidden) { w = saWriteSgrParam(out, w, &first, 8); }
    if (st.strikethrough) { w = saWriteSgrParam(out, w, &first, 9); }

    if (st.fg_mode == SA_COLOR_BASIC) {
        w = saWriteSgrParam(out, w, &first, st.fg_code);
    } else if (st.fg_mode == SA_COLOR_256) {
        w = saWriteSgrParam(out, w, &first, 38);
        w = saWriteSgrParam(out, w, &first, 5);
        w = saWriteSgrParam(out, w, &first, st.fg_code);
    } else if (st.fg_mode == SA_COLOR_RGB) {
        w = saWriteSgrParam(out, w, &first, 38);
        w = saWriteSgrParam(out, w, &first, 2);
        w = saWriteSgrParam(out, w, &first, st.fg_r as u16);
        w = saWriteSgrParam(out, w, &first, st.fg_g as u16);
        w = saWriteSgrParam(out, w, &first, st.fg_b as u16);
    }

    if (st.bg_mode == SA_COLOR_BASIC) {
        w = saWriteSgrParam(out, w, &first, st.bg_code);
    } else if (st.bg_mode == SA_COLOR_256) {
        w = saWriteSgrParam(out, w, &first, 48);
        w = saWriteSgrParam(out, w, &first, 5);
        w = saWriteSgrParam(out, w, &first, st.bg_code);
    } else if (st.bg_mode == SA_COLOR_RGB) {
        w = saWriteSgrParam(out, w, &first, 48);
        w = saWriteSgrParam(out, w, &first, 2);
        w = saWriteSgrParam(out, w, &first, st.bg_r as u16);
        w = saWriteSgrParam(out, w, &first, st.bg_g as u16);
        w = saWriteSgrParam(out, w, &first, st.bg_b as u16);
    }

    w = saWriteByte(out, w, 109); // 'm'
    return w;
}

fn saEmitCloseStyle(out: []u8, pos: usize, st: SaStyleState): usize {
    if (!st.hasAny()) {
        return pos;
    }
    let w: usize = pos;
    if (st.hasAttr()) {
        // Full reset when text attributes (bold/italic/underline/etc.) are active
        w = saWriteByte(out, w, SA_ESC);
        w = saWriteBytes(out, w, "[0m");
        return w;
    }
    if (st.fg_mode != SA_COLOR_NONE) {
        w = saWriteByte(out, w, SA_ESC);
        w = saWriteBytes(out, w, "[39m");
    }
    if (st.bg_mode != SA_COLOR_NONE) {
        w = saWriteByte(out, w, SA_ESC);
        w = saWriteBytes(out, w, "[49m");
    }
    return w;
}

/// Compute the visible terminal column width of `input`, ignoring ANSI escape sequences
/// and accounting for CJK double-width & zero-width Unicode characters.
pub fn ansiVisibleWidth(input: string): usize {
    let total: usize = 0;
    let offset: usize = 0;
    let toks: [64]AnsiToken = undefined;

    while (offset < input.len) {
        const chunk: string = input[offset..input.len];
        const count: usize = tokenizeAnsi(chunk, &toks);
        if (count == 0) {
            break;
        }
        let i: usize = 0;
        while (i < count) {
            const tok: AnsiToken = toks[i];
            if (tok.kind == TOKEN_TEXT) {
                const seg_start: usize = offset + tok.start;
                total += strWidth(input[seg_start..seg_start + tok.len]);
            }
            i += 1;
        }
        const last: AnsiToken = toks[count - 1];
        offset += last.start + last.len;
    }
    return total;
}

/// Slice `input` by visible terminal display columns `[begin, end)` into `out`.
/// Preserves active ANSI SGR styles entering the slice and appends closing/reset
/// sequences (`\x1b[39m` / `\x1b[49m` / `\x1b[0m`) at the end when styles remain active.
/// Returns the number of bytes written to `out`.
pub fn sliceAnsi(input: string, begin: usize, end: usize, out: []u8): usize {
    if (begin >= end || input.len == 0 || out.len == 0) {
        return 0;
    }

    let style: SaStyleState = SaStyleState.empty();
    let col: usize = 0;
    let w: usize = 0;
    let entered: bool = false;
    let done: bool = false;
    let offset: usize = 0;
    let toks: [64]AnsiToken = undefined;

    while (offset < input.len && !done) {
        const chunk: string = input[offset..input.len];
        const count: usize = tokenizeAnsi(chunk, &toks);
        if (count == 0) {
            break;
        }
        let ti: usize = 0;
        while (ti < count && !done) {
            const tok: AnsiToken = toks[ti];
            const abs_start: usize = offset + tok.start;
            const abs_end: usize = abs_start + tok.len;

            if (tok.kind == TOKEN_CSI && tok.final_byte == 109) {
                // SGR sequence (ESC [ ... m)
                const seq: string = input[abs_start..abs_end];
                style.applySgr(seq);
                if (entered && col < end) {
                    w = saWriteBytes(out, w, seq);
                }
            } else if (tok.kind == TOKEN_TEXT) {
                let p: usize = abs_start;
                while (p < abs_end) {
                    const ch: SaUtf8Char = saDecodeUtf8(input, p);
                    const cw: usize = charWidth(ch.cp);

                    if (cw == 0) {
                        if (entered && col <= end) {
                            w = saWriteBytes(out, w, input[p..p + ch.byte_len]);
                        }
                        p += ch.byte_len;
                        continue;
                    }

                    if (col + cw <= begin) {
                        col += cw;
                        p += ch.byte_len;
                        continue;
                    }

                    if (col < begin && col + cw > begin) {
                        // Wide character straddles `begin`: skip to avoid half-character corruption
                        col += cw;
                        p += ch.byte_len;
                        continue;
                    }

                    if (col + cw > end) {
                        done = true;
                        break;
                    }

                    if (!entered) {
                        entered = true;
                        w = saEmitActiveStyle(out, w, style);
                    }
                    w = saWriteBytes(out, w, input[p..p + ch.byte_len]);
                    col += cw;
                    p += ch.byte_len;

                    if (col >= end) {
                        done = true;
                        break;
                    }
                }
            }
            // Non-SGR CSI, OSC (e.g. OSC 8 hyperlinks), and ESC sequences are skipped safely.
            ti += 1;
        }
        const last: AnsiToken = toks[count - 1];
        offset += last.start + last.len;
    }

    if (entered && style.hasAny()) {
        w = saEmitCloseStyle(out, w, style);
    }
    return w;
}

/// Slice `input` from visible terminal display column `begin` to the end into `out`.
pub fn sliceAnsiFrom(input: string, begin: usize, out: []u8): usize {
    return sliceAnsi(input, begin, SA_MAX_END, out);
}

/// Truncate `input` to at most `max_cols` visible terminal columns into `out`.
/// When truncation occurs, appends `ellipsis` after cleanly closing active ANSI styles.
pub fn truncateAnsi(input: string, max_cols: usize, ellipsis: string, out: []u8): usize {
    if (max_cols == 0 || out.len == 0 || input.len == 0) {
        return 0;
    }
    const total_cols: usize = ansiVisibleWidth(input);
    if (total_cols <= max_cols) {
        return sliceAnsi(input, 0, SA_MAX_END, out);
    }
    const ell_cols: usize = ansiVisibleWidth(ellipsis);
    if (ell_cols >= max_cols) {
        return sliceAnsi(ellipsis, 0, max_cols, out);
    }
    const keep_cols: usize = max_cols - ell_cols;
    let w: usize = sliceAnsi(input, 0, keep_cols, out);
    w = saWriteBytes(out, w, ellipsis);
    return w;
}

export {
    ansiVisibleWidth,
    sliceAnsi,
    sliceAnsiFrom,
    truncateAnsi,
};
