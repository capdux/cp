// @cap/boxen — Zero-allocation CLI terminal box renderer for CP
// Clean-room MIT implementation inspired by npm `boxen`.
//
// Features:
// - 6 border styles: Single, Double, Round, Bold, Classic, SingleDouble.
// - Text alignment (Left / Center / Right) & embedded top-border title alignment.
// - Configurable horizontal/vertical padding and left margin.
// - Accurate UTF-8 / CJK fullwidth display width & zero-width ANSI SGR escape handling
//   (`\x1b[...m`) so right borders stay pixel-aligned on every line.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

import { charWidth } from "../../unicode-width/src/index.cp";

pub enum BorderStyle: u8 {
    Single,
    Double,
    Round,
    Bold,
    Classic,
    SingleDouble,
}

pub enum TextAlign: u8 {
    Left,
    Center,
    Right,
}

pub struct BoxOptions {
    pub style: BorderStyle,
    pub text_align: TextAlign,
    pub padding_x: u8,
    pub padding_y: u8,
    pub margin_left: u8,
    pub title: string,
    pub title_align: TextAlign,

    pub static fn defaults() -> BoxOptions {
        return BoxOptions{
            style: BorderStyle.Single,
            text_align: TextAlign.Left,
            padding_x: 1,
            padding_y: 0,
            margin_left: 0,
            title: "",
            title_align: TextAlign.Left,
        };
    }
}

struct BorderChars {
    tl: string,
    top: string,
    tr: string,
    left: string,
    right: string,
    bl: string,
    bottom: string,
    br: string,
}

fn borderForStyle(style: BorderStyle) -> BorderChars {
    if (style == BorderStyle.Double) {
        return BorderChars{ tl: "╔", top: "═", tr: "╗", left: "║", right: "║", bl: "╚", bottom: "═", br: "╝" };
    }
    if (style == BorderStyle.Round) {
        return BorderChars{ tl: "╭", top: "─", tr: "╮", left: "│", right: "│", bl: "╰", bottom: "─", br: "╯" };
    }
    if (style == BorderStyle.Bold) {
        return BorderChars{ tl: "┏", top: "━", tr: "┓", left: "┃", right: "┃", bl: "┗", bottom: "━", br: "┛" };
    }
    if (style == BorderStyle.Classic) {
        return BorderChars{ tl: "+", top: "-", tr: "+", left: "|", right: "|", bl: "+", bottom: "-", br: "+" };
    }
    if (style == BorderStyle.SingleDouble) {
        return BorderChars{ tl: "╓", top: "─", tr: "╖", left: "║", right: "║", bl: "╙", bottom: "─", br: "╜" };
    }
    return BorderChars{ tl: "┌", top: "─", tr: "┐", left: "│", right: "│", bl: "└", bottom: "─", br: "┘" };
}

struct Utf8Step {
    cp: u32,
    width: usize,
}

fn decodeUtf8At(s: string, i: usize) -> Utf8Step {
    const b0 = s[i];
    if (b0 < 0x80) {
        return Utf8Step{ cp: b0 as u32, width: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
        const b1 = s[i + 1];
        if ((b1 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32),
                width: 2,
            };
        }
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32),
                width: 3,
            };
        }
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        const b3 = s[i + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            return Utf8Step{
                cp: (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32),
                width: 4,
            };
        }
    }
    return Utf8Step{ cp: 0xFFFD, width: 1 };
}

fn codepointWidth(cp: u32) -> usize {
    return charWidth(cp);
}

/// Computes terminal display width of a single line, treating CJK/fullwidth/emoji as width 2
/// and ignoring ANSI CSI escape sequences (`\x1b[...m`).
pub fn displayWidth(s: string) -> usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        // Skip ANSI CSI escape sequences: ESC '[' ... (0x40..0x7E)
        if (s[i] == 0x1B && i + 1 < s.len && s[i + 1] == '[') {
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
        const step = decodeUtf8At(s, i);
        w += codepointWidth(step.cp);
        i += step.width;
    }
    return w;
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn pushRepeat(out: []u8, pos: usize, s: string, count: usize) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < count) {
        p = pushStr(out, p, s);
        i += 1;
    }
    return p;
}

fn pushSpaces(out: []u8, pos: usize, count: usize) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < count && p < out.len) {
        out[p] = ' ';
        p += 1;
        i += 1;
    }
    return p;
}

/// Renders `text` inside a customizable terminal box into caller-provided `out` (zero heap allocation).
pub fn renderBox(text: string, opts: BoxOptions, out: []u8) -> string {
    const b = borderForStyle(opts.style);
    const pad_x = opts.padding_x as usize;
    const pad_y = opts.padding_y as usize;
    const margin_l = opts.margin_left as usize;

    // Pass 1: compute max display width across all lines
    let max_line_w: usize = 0;
    let scan: usize = 0;
    while (true) {
        const ls = scan;
        while (scan < text.len && text[scan] != '\n') {
            scan += 1;
        }
        let le = scan;
        if (le > ls && text[le - 1] == '\r') {
            le -= 1;
        }
        const lw = displayWidth(text[ls..le]);
        if (lw > max_line_w) {
            max_line_w = lw;
        }
        if (scan >= text.len) {
            break;
        }
        scan += 1;
    }

    let inner_w = max_line_w + pad_x * 2;
    const has_title = opts.title.len > 0;
    let title_slot: usize = 0;
    if (has_title) {
        title_slot = displayWidth(opts.title) + 2; // " " + title + " "
        if (inner_w < title_slot + 2) {
            inner_w = title_slot + 2;
        }
    }

    let pos: usize = 0;

    // Top border (with optional embedded title)
    pos = pushSpaces(out, pos, margin_l);
    pos = pushStr(out, pos, b.tl);
    if (has_title) {
        const rem_bars = inner_w - title_slot;
        let left_bars: usize = 1;
        if (opts.title_align == TextAlign.Center) {
            left_bars = rem_bars / 2;
        } else if (opts.title_align == TextAlign.Right) {
            left_bars = rem_bars - 1;
        }
        const right_bars = rem_bars - left_bars;
        pos = pushRepeat(out, pos, b.top, left_bars);
        pos = pushByte(out, pos, ' ');
        pos = pushStr(out, pos, opts.title);
        pos = pushByte(out, pos, ' ');
        pos = pushRepeat(out, pos, b.top, right_bars);
    } else {
        pos = pushRepeat(out, pos, b.top, inner_w);
    }
    pos = pushStr(out, pos, b.tr);
    pos = pushByte(out, pos, '\n');

    // Top vertical padding lines
    let py: usize = 0;
    while (py < pad_y) {
        pos = pushSpaces(out, pos, margin_l);
        pos = pushStr(out, pos, b.left);
        pos = pushSpaces(out, pos, inner_w);
        pos = pushStr(out, pos, b.right);
        pos = pushByte(out, pos, '\n');
        py += 1;
    }

    // Content lines
    let cur: usize = 0;
    while (true) {
        const ls = cur;
        while (cur < text.len && text[cur] != '\n') {
            cur += 1;
        }
        let le = cur;
        if (le > ls && text[le - 1] == '\r') {
            le -= 1;
        }
        const line = text[ls..le];
        const lw = displayWidth(line);
        const avail = inner_w - pad_x * 2;
        const extra = if (avail > lw) { avail - lw } else { 0 };

        let pad_l = pad_x;
        let pad_r = pad_x + extra;
        if (opts.text_align == TextAlign.Right) {
            pad_l = pad_x + extra;
            pad_r = pad_x;
        } else if (opts.text_align == TextAlign.Center) {
            const left_extra = extra / 2;
            pad_l = pad_x + left_extra;
            pad_r = pad_x + (extra - left_extra);
        }

        pos = pushSpaces(out, pos, margin_l);
        pos = pushStr(out, pos, b.left);
        pos = pushSpaces(out, pos, pad_l);
        pos = pushStr(out, pos, line);
        pos = pushSpaces(out, pos, pad_r);
        pos = pushStr(out, pos, b.right);
        pos = pushByte(out, pos, '\n');

        if (cur >= text.len) {
            break;
        }
        cur += 1;
    }

    // Bottom vertical padding lines
    py = 0;
    while (py < pad_y) {
        pos = pushSpaces(out, pos, margin_l);
        pos = pushStr(out, pos, b.left);
        pos = pushSpaces(out, pos, inner_w);
        pos = pushStr(out, pos, b.right);
        pos = pushByte(out, pos, '\n');
        py += 1;
    }

    // Bottom border
    pos = pushSpaces(out, pos, margin_l);
    pos = pushStr(out, pos, b.bl);
    pos = pushRepeat(out, pos, b.bottom, inner_w);
    pos = pushStr(out, pos, b.br);

    return out[0..pos];
}

/// Renders `text` inside a default single-line box into caller-provided `out`.
pub fn box(text: string, out: []u8) -> string {
    return renderBox(text, BoxOptions.defaults(), out);
}

/// Renders `text` inside a titled box with the chosen `BorderStyle` into caller-provided `out`.
pub fn titledBox(title: string, text: string, style: BorderStyle, out: []u8) -> string {
    let opts = BoxOptions.defaults();
    opts.title = title;
    opts.style = style;
    return renderBox(text, opts, out);
}
