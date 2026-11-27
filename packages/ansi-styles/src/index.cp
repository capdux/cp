// @cap/ansi-styles — ANSI escape codes for styling strings in the terminal
// Clean-room MIT CP implementation of npm `ansi-styles` (400M+/wk, core dependency of `chalk`)
// Zero-heap, reuses `@cap/color-convert` for Hex/Keyword/RGB -> ANSI16 / ANSI256 / Truecolor conversion.

import { rgbToAnsi256, rgbToAnsi16, hexToRgb, keywordToRgb } from "../../color-convert/src/index.cp";

pub struct StylePair {
    pub open: string,
    pub close: string,
}

// Modifier SGR codes
pub const RESET: StylePair = StylePair { open: "\x1b[0m", close: "\x1b[0m" };
pub const BOLD: StylePair = StylePair { open: "\x1b[1m", close: "\x1b[22m" };
pub const DIM: StylePair = StylePair { open: "\x1b[2m", close: "\x1b[22m" };
pub const ITALIC: StylePair = StylePair { open: "\x1b[3m", close: "\x1b[23m" };
pub const UNDERLINE: StylePair = StylePair { open: "\x1b[4m", close: "\x1b[24m" };
pub const OVERLINE: StylePair = StylePair { open: "\x1b[53m", close: "\x1b[55m" };
pub const INVERSE: StylePair = StylePair { open: "\x1b[7m", close: "\x1b[27m" };
pub const HIDDEN: StylePair = StylePair { open: "\x1b[8m", close: "\x1b[28m" };
pub const STRIKETHROUGH: StylePair = StylePair { open: "\x1b[9m", close: "\x1b[29m" };

// Foreground standard & bright colors (close = \x1b[39m)
pub const FG_BLACK: StylePair = StylePair { open: "\x1b[30m", close: "\x1b[39m" };
pub const FG_RED: StylePair = StylePair { open: "\x1b[31m", close: "\x1b[39m" };
pub const FG_GREEN: StylePair = StylePair { open: "\x1b[32m", close: "\x1b[39m" };
pub const FG_YELLOW: StylePair = StylePair { open: "\x1b[33m", close: "\x1b[39m" };
pub const FG_BLUE: StylePair = StylePair { open: "\x1b[34m", close: "\x1b[39m" };
pub const FG_MAGENTA: StylePair = StylePair { open: "\x1b[35m", close: "\x1b[39m" };
pub const FG_CYAN: StylePair = StylePair { open: "\x1b[36m", close: "\x1b[39m" };
pub const FG_WHITE: StylePair = StylePair { open: "\x1b[37m", close: "\x1b[39m" };
pub const FG_GRAY: StylePair = StylePair { open: "\x1b[90m", close: "\x1b[39m" };
pub const FG_RED_BRIGHT: StylePair = StylePair { open: "\x1b[91m", close: "\x1b[39m" };
pub const FG_GREEN_BRIGHT: StylePair = StylePair { open: "\x1b[92m", close: "\x1b[39m" };
pub const FG_YELLOW_BRIGHT: StylePair = StylePair { open: "\x1b[93m", close: "\x1b[39m" };
pub const FG_BLUE_BRIGHT: StylePair = StylePair { open: "\x1b[94m", close: "\x1b[39m" };
pub const FG_MAGENTA_BRIGHT: StylePair = StylePair { open: "\x1b[95m", close: "\x1b[39m" };
pub const FG_CYAN_BRIGHT: StylePair = StylePair { open: "\x1b[96m", close: "\x1b[39m" };
pub const FG_WHITE_BRIGHT: StylePair = StylePair { open: "\x1b[97m", close: "\x1b[39m" };

// Background standard & bright colors (close = \x1b[49m)
pub const BG_BLACK: StylePair = StylePair { open: "\x1b[40m", close: "\x1b[49m" };
pub const BG_RED: StylePair = StylePair { open: "\x1b[41m", close: "\x1b[49m" };
pub const BG_GREEN: StylePair = StylePair { open: "\x1b[42m", close: "\x1b[49m" };
pub const BG_YELLOW: StylePair = StylePair { open: "\x1b[43m", close: "\x1b[49m" };
pub const BG_BLUE: StylePair = StylePair { open: "\x1b[44m", close: "\x1b[49m" };
pub const BG_MAGENTA: StylePair = StylePair { open: "\x1b[45m", close: "\x1b[49m" };
pub const BG_CYAN: StylePair = StylePair { open: "\x1b[46m", close: "\x1b[49m" };
pub const BG_WHITE: StylePair = StylePair { open: "\x1b[47m", close: "\x1b[49m" };
pub const BG_GRAY: StylePair = StylePair { open: "\x1b[100m", close: "\x1b[49m" };

pub const FG_CLOSE: string = "\x1b[39m";
pub const BG_CLOSE: string = "\x1b[49m";

fn asPushStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn asPushU8(out: []u8, pos: usize, v: u8): usize {
    let p = pos;
    if (v >= 100) {
        if (p < out.len) { out[p] = 48 + (v / 100); p = p + 1; }
        if (p < out.len) { out[p] = 48 + ((v / 10) % 10); p = p + 1; }
        if (p < out.len) { out[p] = 48 + (v % 10); p = p + 1; }
    } else if (v >= 10) {
        if (p < out.len) { out[p] = 48 + (v / 10); p = p + 1; }
        if (p < out.len) { out[p] = 48 + (v % 10); p = p + 1; }
    } else {
        if (p < out.len) { out[p] = 48 + v; p = p + 1; }
    }
    return p;
}

/// Wraps `text` with `style.open` + `text` + `style.close`.
pub fn wrapStyle(style: StylePair, text: string, out: []u8): usize {
    let p: usize = 0;
    p = asPushStr(out, p, style.open);
    p = asPushStr(out, p, text);
    p = asPushStr(out, p, style.close);
    return p;
}

/// Formats a 256-color SGR opening escape sequence: `\x1b[38;5;<n>m` (fg) or `\x1b[48;5;<n>m` (bg).
pub fn formatAnsi256(code: u8, is_bg: bool, out: []u8): usize {
    let p: usize = 0;
    p = asPushStr(out, p, if (is_bg) { "\x1b[48;5;" } else { "\x1b[38;5;" });
    p = asPushU8(out, p, code);
    if (p < out.len) { out[p] = 109; p = p + 1; } // 'm'
    return p;
}

/// Formats a 24-bit Truecolor SGR opening sequence: `\x1b[38;2;<r>;<g>;<b>m` (fg) or `\x1b[48;2;...m` (bg).
pub fn formatAnsi16m(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize {
    let p: usize = 0;
    p = asPushStr(out, p, if (is_bg) { "\x1b[48;2;" } else { "\x1b[38;2;" });
    p = asPushU8(out, p, r);
    if (p < out.len) { out[p] = 59; p = p + 1; } // ';'
    p = asPushU8(out, p, g);
    if (p < out.len) { out[p] = 59; p = p + 1; }
    p = asPushU8(out, p, b);
    if (p < out.len) { out[p] = 109; p = p + 1; } // 'm'
    return p;
}

/// Formats an ANSI 16-color SGR opening sequence from RGB via `@cap/color-convert`.
pub fn formatRgbToAnsi16(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize {
    let code = rgbToAnsi16(r, g, b);
    let final_code = if (is_bg) { code + 10 } else { code };
    let p: usize = 0;
    p = asPushStr(out, p, "\x1b[");
    p = asPushU8(out, p, final_code);
    if (p < out.len) { out[p] = 109; p = p + 1; }
    return p;
}

/// Formats a 256-color SGR opening sequence from RGB via `@cap/color-convert`.
pub fn formatRgbToAnsi256(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize {
    let code = rgbToAnsi256(r, g, b);
    return formatAnsi256(code, is_bg, out);
}

/// Formats a Truecolor SGR opening sequence from a Hex string (e.g. `"#FF8800"`) via `@cap/color-convert`.
/// Returns 0 if `hex` is invalid.
pub fn formatHexToAnsi16m(hex: string, is_bg: bool, out: []u8): usize {
    let c = hexToRgb(hex);
    if (!c.found) { return 0; }
    return formatAnsi16m(c.r, c.g, c.b, is_bg, out);
}

/// Formats a Truecolor SGR opening sequence from a CSS4 color keyword (e.g. `"rebeccapurple"`) via `@cap/color-convert` -> `@cap/color-name`.
/// Returns 0 if `name` is unknown.
pub fn formatKeywordToAnsi16m(name: string, is_bg: bool, out: []u8): usize {
    let c = keywordToRgb(name);
    if (!c.found) { return 0; }
    return formatAnsi16m(c.r, c.g, c.b, is_bg, out);
}
