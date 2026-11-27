// @cap/chalk — Zero-allocation terminal string styling library for CP
// Clean-room MIT implementation with parity to npm `chalk` v5.
// Composes `@cap/ansi-styles` (-> `@cap/color-convert` -> `@cap/color-name`)
// and `@cap/supports-color` (-> `@cap/has-flag`) with zero duplication.

import {
    StylePair,
    BOLD,
    DIM,
    ITALIC,
    UNDERLINE,
    INVERSE,
    STRIKETHROUGH,
    FG_BLACK,
    FG_RED,
    FG_GREEN,
    FG_YELLOW,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_WHITE,
    FG_GRAY,
    BG_BLACK,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE,
    BG_GRAY,
    FG_CLOSE,
    BG_CLOSE,
    wrapStyle,
    formatAnsi256,
    formatAnsi16m,
    formatRgbToAnsi16,
    formatRgbToAnsi256,
    formatHexToAnsi16m,
    formatKeywordToAnsi16m,
} from "../../ansi-styles/src/index.cp";
import {
    hexToRgb,
    keywordToRgb,
} from "../../color-convert/src/index.cp";
import {
    COLOR_LEVEL_NONE,
    COLOR_LEVEL_BASIC,
    COLOR_LEVEL_256,
    COLOR_LEVEL_16M,
    TerminalEnv,
    detectColorLevel,
} from "../../supports-color/src/index.cp";
import { writeStr } from "cap:fmt";

fn chkCopyPlain(text: string, out: []u8): usize {
    return writeStr(text, out, 0);
}

fn chkWrapOpenClose(open: string, text: string, close: string, out: []u8): usize {
    let p: usize = 0;
    p = writeStr(open, out, p);
    p = writeStr(text, out, p);
    p = writeStr(close, out, p);
    return p;
}

fn chkApplyPair(level: u8, style: StylePair, text: string, out: []u8): usize {
    if (level == COLOR_LEVEL_NONE) {
        return chkCopyPlain(text, out);
    }
    return wrapStyle(style, text, out);
}

/// Terminal string styler with automatic 4-level color support (`0`=none, `1`=16, `2`=256, `3`=16M).
pub struct Chalk {
    pub level: u8,

    /// Construct a `Chalk` instance with explicit color level (`0..=3`).
    pub static fn new(level: u8): Chalk {
        let lv = level;
        if (lv > COLOR_LEVEL_16M) {
            lv = COLOR_LEVEL_16M;
        }
        return Chalk { level: lv };
    }

    /// Construct a `Chalk` instance by auto-detecting terminal capability via `@cap/supports-color`.
    pub static fn fromEnv(env: TerminalEnv, argv: []const string): Chalk {
        return Chalk.new(detectColorLevel(env, argv));
    }

    // --- Foreground basic colors ---
    pub fn black(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_BLACK, text, out); }
    pub fn red(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_RED, text, out); }
    pub fn green(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_GREEN, text, out); }
    pub fn yellow(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_YELLOW, text, out); }
    pub fn blue(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_BLUE, text, out); }
    pub fn magenta(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_MAGENTA, text, out); }
    pub fn cyan(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_CYAN, text, out); }
    pub fn white(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_WHITE, text, out); }
    pub fn gray(text: string, out: []u8): usize { return chkApplyPair(_.level, FG_GRAY, text, out); }

    // --- Background basic colors ---
    pub fn bgBlack(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_BLACK, text, out); }
    pub fn bgRed(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_RED, text, out); }
    pub fn bgGreen(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_GREEN, text, out); }
    pub fn bgYellow(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_YELLOW, text, out); }
    pub fn bgBlue(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_BLUE, text, out); }
    pub fn bgMagenta(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_MAGENTA, text, out); }
    pub fn bgCyan(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_CYAN, text, out); }
    pub fn bgWhite(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_WHITE, text, out); }
    pub fn bgGray(text: string, out: []u8): usize { return chkApplyPair(_.level, BG_GRAY, text, out); }

    // --- Text modifiers ---
    pub fn bold(text: string, out: []u8): usize { return chkApplyPair(_.level, BOLD, text, out); }
    pub fn dim(text: string, out: []u8): usize { return chkApplyPair(_.level, DIM, text, out); }
    pub fn italic(text: string, out: []u8): usize { return chkApplyPair(_.level, ITALIC, text, out); }
    pub fn underline(text: string, out: []u8): usize { return chkApplyPair(_.level, UNDERLINE, text, out); }
    pub fn inverse(text: string, out: []u8): usize { return chkApplyPair(_.level, INVERSE, text, out); }
    pub fn strikethrough(text: string, out: []u8): usize { return chkApplyPair(_.level, STRIKETHROUGH, text, out); }

    // --- 256-color direct ---
    pub fn ansi256(code: u8, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        let openBuf: [32]u8 = undefined;
        const n = formatAnsi256(code, false, openBuf[0..32]);
        return chkWrapOpenClose(openBuf[0..n], text, FG_CLOSE, out);
    }

    pub fn bgAnsi256(code: u8, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        let openBuf: [32]u8 = undefined;
        const n = formatAnsi256(code, true, openBuf[0..32]);
        return chkWrapOpenClose(openBuf[0..n], text, BG_CLOSE, out);
    }

    /// Foreground RGB color with automatic 4-level downsampling (`16M -> 256 -> 16 -> plain`).
    pub fn rgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        let openBuf: [32]u8 = undefined;
        let n: usize = 0;
        if (_.level >= COLOR_LEVEL_16M) {
            n = formatAnsi16m(r, g, b, false, openBuf[0..32]);
        } else if (_.level == COLOR_LEVEL_256) {
            n = formatRgbToAnsi256(r, g, b, false, openBuf[0..32]);
        } else {
            n = formatRgbToAnsi16(r, g, b, false, openBuf[0..32]);
        }
        return chkWrapOpenClose(openBuf[0..n], text, FG_CLOSE, out);
    }

    /// Background RGB color with automatic 4-level downsampling (`16M -> 256 -> 16 -> plain`).
    pub fn bgRgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        let openBuf: [32]u8 = undefined;
        let n: usize = 0;
        if (_.level >= COLOR_LEVEL_16M) {
            n = formatAnsi16m(r, g, b, true, openBuf[0..32]);
        } else if (_.level == COLOR_LEVEL_256) {
            n = formatRgbToAnsi256(r, g, b, true, openBuf[0..32]);
        } else {
            n = formatRgbToAnsi16(r, g, b, true, openBuf[0..32]);
        }
        return chkWrapOpenClose(openBuf[0..n], text, BG_CLOSE, out);
    }

    /// Foreground CSS Hex color (e.g. `"#FF8800"`) with automatic 4-level downsampling.
    pub fn hex(hex_str: string, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        if (_.level >= COLOR_LEVEL_16M) {
            let openBuf: [32]u8 = undefined;
            const n = formatHexToAnsi16m(hex_str, false, openBuf[0..32]);
            if (n == 0) {
                return chkCopyPlain(text, out);
            }
            return chkWrapOpenClose(openBuf[0..n], text, FG_CLOSE, out);
        }
        const c = hexToRgb(hex_str);
        if (!c.found) {
            return chkCopyPlain(text, out);
        }
        return _.rgb(c.r, c.g, c.b, text, out);
    }

    /// Background CSS Hex color with automatic 4-level downsampling.
    pub fn bgHex(hex_str: string, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        if (_.level >= COLOR_LEVEL_16M) {
            let openBuf: [32]u8 = undefined;
            const n = formatHexToAnsi16m(hex_str, true, openBuf[0..32]);
            if (n == 0) {
                return chkCopyPlain(text, out);
            }
            return chkWrapOpenClose(openBuf[0..n], text, BG_CLOSE, out);
        }
        const c = hexToRgb(hex_str);
        if (!c.found) {
            return chkCopyPlain(text, out);
        }
        return _.bgRgb(c.r, c.g, c.b, text, out);
    }

    /// Foreground CSS4 named color (e.g. `"rebeccapurple"`, `"orange"`) with automatic 4-level downsampling.
    pub fn keyword(name: string, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        if (_.level >= COLOR_LEVEL_16M) {
            let openBuf: [32]u8 = undefined;
            const n = formatKeywordToAnsi16m(name, false, openBuf[0..32]);
            if (n == 0) {
                return chkCopyPlain(text, out);
            }
            return chkWrapOpenClose(openBuf[0..n], text, FG_CLOSE, out);
        }
        const c = keywordToRgb(name);
        if (!c.found) {
            return chkCopyPlain(text, out);
        }
        return _.rgb(c.r, c.g, c.b, text, out);
    }

    /// Background CSS4 named color with automatic 4-level downsampling.
    pub fn bgKeyword(name: string, text: string, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        if (_.level >= COLOR_LEVEL_16M) {
            let openBuf: [32]u8 = undefined;
            const n = formatKeywordToAnsi16m(name, true, openBuf[0..32]);
            if (n == 0) {
                return chkCopyPlain(text, out);
            }
            return chkWrapOpenClose(openBuf[0..n], text, BG_CLOSE, out);
        }
        const c = keywordToRgb(name);
        if (!c.found) {
            return chkCopyPlain(text, out);
        }
        return _.bgRgb(c.r, c.g, c.b, text, out);
    }

    /// Compose optional bold, underline, and foreground `StylePair` in one zero-allocation pass.
    pub fn styled(text: string, is_bold: bool, is_underline: bool, fg: StylePair, out: []u8): usize {
        if (_.level == COLOR_LEVEL_NONE) {
            return chkCopyPlain(text, out);
        }
        let p: usize = 0;
        if (is_bold) {
            p = writeStr(BOLD.open, out, p);
        }
        if (is_underline) {
            p = writeStr(UNDERLINE.open, out, p);
        }
        if (fg.open.len > 0) {
            p = writeStr(fg.open, out, p);
        }
        p = writeStr(text, out, p);
        if (fg.close.len > 0) {
            p = writeStr(fg.close, out, p);
        }
        if (is_underline) {
            p = writeStr(UNDERLINE.close, out, p);
        }
        if (is_bold) {
            p = writeStr(BOLD.close, out, p);
        }
        return p;
    }
}

export {
    COLOR_LEVEL_NONE,
    COLOR_LEVEL_BASIC,
    COLOR_LEVEL_256,
    COLOR_LEVEL_16M,
    TerminalEnv,
    StylePair,
    FG_RED,
    FG_GREEN,
    FG_YELLOW,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_WHITE,
    FG_GRAY,
    Chalk,
};
