// @cap/boxen-ui — Zero-allocation terminal Unicode box layout renderer for CP
// Clean-room MIT implementation with parity to npm `boxen` v8.
// Composes `@cap/string-width` for accurate ANSI escape & CJK wide-character alignment.

import {
    stringWidth,
    maxLineWidth,
} from "../../string-width/src/index.cp";

pub const BORDER_SINGLE: u8 = 0;
pub const BORDER_DOUBLE: u8 = 1;
pub const BORDER_ROUND: u8 = 2;
pub const BORDER_BOLD: u8 = 3;
pub const BORDER_CLASSIC: u8 = 4;

pub const ALIGN_LEFT: u8 = 0;
pub const ALIGN_CENTER: u8 = 1;
pub const ALIGN_RIGHT: u8 = 2;

/// Configuration options for `renderBox`.
pub struct BoxenOptions {
    pub border_style: u8,
    pub padding_x: usize,
    pub padding_y: usize,
    pub text_align: u8,
    pub title: string,

    /// Return default `boxen` options:
    /// single-line border, 0 padding, left alignment, no title.
    pub static fn defaultOpts(): BoxenOptions {
        return BoxenOptions {
            border_style: BORDER_SINGLE,
            padding_x: 0,
            padding_y: 0,
            text_align: ALIGN_LEFT,
            title: "",
        };
    }
}

struct BxnBorder {
    tl: string,
    horiz: string,
    tr: string,
    vert: string,
    bl: string,
    br: string,
}

fn bxnGetBorder(style: u8): BxnBorder {
    if (style == BORDER_DOUBLE) {
        return BxnBorder {
            tl: "╔",
            horiz: "═",
            tr: "╗",
            vert: "║",
            bl: "╚",
            br: "╝",
        };
    }
    if (style == BORDER_ROUND) {
        return BxnBorder {
            tl: "╭",
            horiz: "─",
            tr: "╮",
            vert: "│",
            bl: "╰",
            br: "╯",
        };
    }
    if (style == BORDER_BOLD) {
        return BxnBorder {
            tl: "┏",
            horiz: "━",
            tr: "┓",
            vert: "┃",
            bl: "┗",
            br: "┛",
        };
    }
    if (style == BORDER_CLASSIC) {
        return BxnBorder {
            tl: "+",
            horiz: "-",
            tr: "+",
            vert: "|",
            bl: "+",
            br: "+",
        };
    }
    return BxnBorder {
        tl: "┌",
        horiz: "─",
        tr: "┐",
        vert: "│",
        bl: "└",
        br: "┘",
    };
}

fn bxnWriteByte(dst: []u8, pos: usize, ch: u8): usize {
    if (pos < dst.len) {
        dst[pos] = ch;
        return pos + 1;
    }
    return pos;
}

fn bxnWriteStr(dst: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < dst.len) {
        dst[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn bxnRepeatByte(dst: []u8, pos: usize, ch: u8, count: usize): usize {
    let w = pos;
    let i: usize = 0;
    while (i < count && w < dst.len) {
        dst[w] = ch;
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn bxnRepeatStr(dst: []u8, pos: usize, s: string, count: usize): usize {
    let w = pos;
    let i: usize = 0;
    while (i < count && w < dst.len) {
        w = bxnWriteStr(dst, w, s);
        i = i + 1;
    }
    return w;
}

fn bxnWriteEmptyRow(dst: []u8, pos: usize, border: BxnBorder, inner_w: usize): usize {
    let w = bxnWriteStr(dst, pos, border.vert);
    w = bxnRepeatByte(dst, w, 32, inner_w);
    w = bxnWriteStr(dst, w, border.vert);
    w = bxnWriteByte(dst, w, 10);
    return w;
}

fn bxnWriteContentLine(
    dst: []u8,
    pos: usize,
    border: BxnBorder,
    line: string,
    inner_w: usize,
    padding_x: usize,
    text_align: u8
): usize {
    const line_w = stringWidth(line);
    const used = padding_x * 2 + line_w;
    const avail: usize = if (inner_w > used) inner_w - used else 0;

    let left_extra: usize = 0;
    let right_extra: usize = avail;
    if (text_align == ALIGN_CENTER) {
        left_extra = avail / 2;
        right_extra = avail - left_extra;
    } else if (text_align == ALIGN_RIGHT) {
        left_extra = avail;
        right_extra = 0;
    }

    let w = bxnWriteStr(dst, pos, border.vert);
    w = bxnRepeatByte(dst, w, 32, padding_x + left_extra);
    w = bxnWriteStr(dst, w, line);
    w = bxnRepeatByte(dst, w, 32, right_extra + padding_x);
    w = bxnWriteStr(dst, w, border.vert);
    w = bxnWriteByte(dst, w, 10);
    return w;
}

/// Render `text` inside a Unicode/ASCII box according to `opts`, writing UTF-8 bytes
/// directly into caller-provided `out` with zero heap allocations.
/// Returns total bytes written into `out`.
pub fn renderBox(text: string, opts: BoxenOptions, out: []u8): usize {
    const border = bxnGetBorder(opts.border_style);
    const content_w = maxLineWidth(text);
    let inner_w = content_w + opts.padding_x * 2;

    const has_title = opts.title.len > 0;
    let title_w: usize = 0;
    if (has_title) {
        title_w = stringWidth(opts.title);
        // Minimum top border with embedded title: "─ " + title + " ─" => title_w + 4 columns
        const min_title_inner = title_w + 4;
        if (inner_w < min_title_inner) {
            inner_w = min_title_inner;
        }
    }

    // 1. Top border (with optional embedded title: ┌─ Title ─┐)
    let pos = bxnWriteStr(out, 0, border.tl);
    if (has_title) {
        pos = bxnWriteStr(out, pos, border.horiz);
        pos = bxnWriteByte(out, pos, 32);
        pos = bxnWriteStr(out, pos, opts.title);
        pos = bxnWriteByte(out, pos, 32);
        const right_dashes: usize = if (inner_w > title_w + 3) inner_w - title_w - 3 else 0;
        pos = bxnRepeatStr(out, pos, border.horiz, right_dashes);
    } else {
        pos = bxnRepeatStr(out, pos, border.horiz, inner_w);
    }
    pos = bxnWriteStr(out, pos, border.tr);
    pos = bxnWriteByte(out, pos, 10);

    // 2. Top vertical padding rows
    let py: usize = 0;
    while (py < opts.padding_y) {
        pos = bxnWriteEmptyRow(out, pos, border, inner_w);
        py = py + 1;
    }

    // 3. Content lines (split on '\n', strip trailing '\r')
    let start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) {
        if (i == text.len || text[i] == 10) {
            let end = i;
            if (end > start && text[end - 1] == 13) {
                end = end - 1;
            }
            pos = bxnWriteContentLine(
                out,
                pos,
                border,
                text[start..end],
                inner_w,
                opts.padding_x,
                opts.text_align
            );
            start = i + 1;
        }
        i = i + 1;
    }

    // 4. Bottom vertical padding rows
    py = 0;
    while (py < opts.padding_y) {
        pos = bxnWriteEmptyRow(out, pos, border, inner_w);
        py = py + 1;
    }

    // 5. Bottom border
    pos = bxnWriteStr(out, pos, border.bl);
    pos = bxnRepeatStr(out, pos, border.horiz, inner_w);
    pos = bxnWriteStr(out, pos, border.br);

    return pos;
}

export {
    BORDER_SINGLE,
    BORDER_DOUBLE,
    BORDER_ROUND,
    BORDER_BOLD,
    BORDER_CLASSIC,
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    BoxenOptions,
    renderBox,
};
