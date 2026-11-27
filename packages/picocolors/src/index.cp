// @cap/picocolors — Pure CP zero-allocation terminal ANSI color formatter.
// Clean-room MIT implementation inspired by npm `picocolors` v1.
// Reuses ANSI SGR StylePair definitions from `@cap/ansi-styles` with zero duplication.

import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
import {
    StylePair,
    RESET,
    BOLD,
    DIM,
    ITALIC,
    UNDERLINE,
    INVERSE,
    HIDDEN,
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
    BG_WHITE
} from "../../ansi-styles/src/index.cp";

pub const PICO_BUF_CAP: usize = 1024;

fn picoMatchAt(haystack: string, offset: usize, needle: string): bool {
    if (needle.len == 0 || offset + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[offset + i] != needle[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

// Formats `text` with `pair`, replacing any embedded occurrences of `pair.close`
// with `replace_with` (picocolors nested close-code recovery).
fn picoFormatPair(
    enabled: bool,
    pair: StylePair,
    replace_with: string,
    text: string,
    out: []u8
): usize {
    if (text.len == 0 || out.len == 0) {
        return 0;
    }
    if (!enabled) {
        return writeStr(text, out, 0);
    }

    let p: usize = 0;
    p = writeStr(pair.open, out, p);

    let i: usize = 0;
    while (i < text.len && p < out.len) {
        if (pair.close.len > 0 && picoMatchAt(text, i, pair.close)) {
            p = writeStr(replace_with, out, p);
            i = i + pair.close.len;
        } else {
            out[p] = text[i];
            p = p + 1;
            i = i + 1;
        }
    }

    p = writeStr(pair.close, out, p);
    return p;
}

/// Detects terminal color support following `FORCE_COLOR`, `NO_COLOR`, and TTY rules.
/// - `FORCE_COLOR="0"` disables colors; any other non-empty `FORCE_COLOR` enables colors.
/// - Non-empty `NO_COLOR` disables colors (per no-color.org).
/// - Otherwise falls back to `is_tty`.
pub fn detectColorSupport(no_color: string, force_color: string, is_tty: bool): bool {
    if (force_color.len > 0) {
        if (strEq(force_color, "0")) {
            return false;
        }
        return true;
    }
    if (no_color.len > 0) {
        return false;
    }
    return is_tty;
}

/// Color support configuration for `PicoFormatter` and `PicoBuf`.
pub struct ColorConfig {
    pub is_color_supported: bool,

    pub static fn enabled(): ColorConfig {
        return ColorConfig { is_color_supported: true };
    }

    pub static fn disabled(): ColorConfig {
        return ColorConfig { is_color_supported: false };
    }

    pub static fn fromEnv(no_color: string, force_color: string, is_tty: bool): ColorConfig {
        return ColorConfig {
            is_color_supported: detectColorSupport(no_color, force_color, is_tty)
        };
    }
}

/// Stateless zero-heap ANSI formatter writing directly into caller-provided `[]u8` buffers.
pub struct PicoFormatter {
    pub cfg: ColorConfig,

    pub static fn new(is_color_supported: bool): PicoFormatter {
        return PicoFormatter {
            cfg: ColorConfig { is_color_supported: is_color_supported }
        };
    }

    pub static fn fromConfig(cfg: ColorConfig): PicoFormatter {
        return PicoFormatter { cfg: cfg };
    }

    pub fn isColorSupported(): bool {
        return _.cfg.is_color_supported;
    }

    pub fn style(pair: StylePair, text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, pair, pair.open, text, out);
    }

    pub fn reset(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, RESET, RESET.open, text, out);
    }

    pub fn bold(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BOLD, "\x1b[22m\x1b[1m", text, out);
    }

    pub fn dim(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, DIM, "\x1b[22m\x1b[2m", text, out);
    }

    pub fn italic(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, ITALIC, ITALIC.open, text, out);
    }

    pub fn underline(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, UNDERLINE, UNDERLINE.open, text, out);
    }

    pub fn inverse(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, INVERSE, INVERSE.open, text, out);
    }

    pub fn hidden(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, HIDDEN, HIDDEN.open, text, out);
    }

    pub fn strikethrough(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, STRIKETHROUGH, STRIKETHROUGH.open, text, out);
    }

    pub fn black(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_BLACK, FG_BLACK.open, text, out);
    }

    pub fn red(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_RED, FG_RED.open, text, out);
    }

    pub fn green(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_GREEN, FG_GREEN.open, text, out);
    }

    pub fn yellow(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_YELLOW, FG_YELLOW.open, text, out);
    }

    pub fn blue(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_BLUE, FG_BLUE.open, text, out);
    }

    pub fn magenta(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_MAGENTA, FG_MAGENTA.open, text, out);
    }

    pub fn cyan(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_CYAN, FG_CYAN.open, text, out);
    }

    pub fn white(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_WHITE, FG_WHITE.open, text, out);
    }

    pub fn gray(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, FG_GRAY, FG_GRAY.open, text, out);
    }

    pub fn bgBlack(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_BLACK, BG_BLACK.open, text, out);
    }

    pub fn bgRed(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_RED, BG_RED.open, text, out);
    }

    pub fn bgGreen(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_GREEN, BG_GREEN.open, text, out);
    }

    pub fn bgYellow(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_YELLOW, BG_YELLOW.open, text, out);
    }

    pub fn bgBlue(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_BLUE, BG_BLUE.open, text, out);
    }

    pub fn bgMagenta(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_MAGENTA, BG_MAGENTA.open, text, out);
    }

    pub fn bgCyan(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_CYAN, BG_CYAN.open, text, out);
    }

    pub fn bgWhite(text: string, out: []u8): usize {
        return picoFormatPair(_.cfg.is_color_supported, BG_WHITE, BG_WHITE.open, text, out);
    }
}

/// Fixed-capacity (1024-byte) zero-heap accumulating ANSI color buffer with safe truncation.
pub struct PicoBuf {
    pub cfg: ColorConfig,
    pub buf: [1024]u8,
    pub len: usize,

    pub static fn new(is_color_supported: bool): PicoBuf {
        return PicoBuf {
            cfg: ColorConfig { is_color_supported: is_color_supported },
            buf: [0; 1024],
            len: 0
        };
    }

    pub static fn fromConfig(cfg: ColorConfig): PicoBuf {
        return PicoBuf {
            cfg: cfg,
            buf: [0; 1024],
            len: 0
        };
    }

    pub fn clear(*_) {
        _.len = 0;
    }

    pub fn pushPlain(*_, text: string): usize {
        let next_pos = writeStr(text, _.buf[0..PICO_BUF_CAP], _.len);
        let added = next_pos - _.len;
        _.len = next_pos;
        return added;
    }

    pub fn pushStyled(*_, pair: StylePair, text: string): usize {
        if (_.len >= PICO_BUF_CAP) {
            return 0;
        }
        let added = picoFormatPair(
            _.cfg.is_color_supported,
            pair,
            pair.open,
            text,
            _.buf[_.len..PICO_BUF_CAP]
        );
        _.len = _.len + added;
        return added;
    }

    pub fn bold(*_, text: string): usize {
        if (_.len >= PICO_BUF_CAP) { return 0; }
        let added = picoFormatPair(_.cfg.is_color_supported, BOLD, "\x1b[22m\x1b[1m", text, _.buf[_.len..PICO_BUF_CAP]);
        _.len = _.len + added;
        return added;
    }

    pub fn dim(*_, text: string): usize {
        if (_.len >= PICO_BUF_CAP) { return 0; }
        let added = picoFormatPair(_.cfg.is_color_supported, DIM, "\x1b[22m\x1b[2m", text, _.buf[_.len..PICO_BUF_CAP]);
        _.len = _.len + added;
        return added;
    }

    pub fn italic(*_, text: string): usize {
        return _.pushStyled(ITALIC, text);
    }

    pub fn underline(*_, text: string): usize {
        return _.pushStyled(UNDERLINE, text);
    }

    pub fn inverse(*_, text: string): usize {
        return _.pushStyled(INVERSE, text);
    }

    pub fn strikethrough(*_, text: string): usize {
        return _.pushStyled(STRIKETHROUGH, text);
    }

    pub fn black(*_, text: string): usize {
        return _.pushStyled(FG_BLACK, text);
    }

    pub fn red(*_, text: string): usize {
        return _.pushStyled(FG_RED, text);
    }

    pub fn green(*_, text: string): usize {
        return _.pushStyled(FG_GREEN, text);
    }

    pub fn yellow(*_, text: string): usize {
        return _.pushStyled(FG_YELLOW, text);
    }

    pub fn blue(*_, text: string): usize {
        return _.pushStyled(FG_BLUE, text);
    }

    pub fn magenta(*_, text: string): usize {
        return _.pushStyled(FG_MAGENTA, text);
    }

    pub fn cyan(*_, text: string): usize {
        return _.pushStyled(FG_CYAN, text);
    }

    pub fn white(*_, text: string): usize {
        return _.pushStyled(FG_WHITE, text);
    }

    pub fn gray(*_, text: string): usize {
        return _.pushStyled(FG_GRAY, text);
    }

    pub fn bgRed(*_, text: string): usize {
        return _.pushStyled(BG_RED, text);
    }

    pub fn bgGreen(*_, text: string): usize {
        return _.pushStyled(BG_GREEN, text);
    }

    pub fn bgYellow(*_, text: string): usize {
        return _.pushStyled(BG_YELLOW, text);
    }

    pub fn bgBlue(*_, text: string): usize {
        return _.pushStyled(BG_BLUE, text);
    }
}

export {
    PICO_BUF_CAP,
    detectColorSupport,
    ColorConfig,
    PicoFormatter,
    PicoBuf
};
