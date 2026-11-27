// @cap/colorette — Pure CP zero-allocation terminal ANSI color formatter.
// Clean-room MIT implementation inspired by npm `colorette` v2.
// Reuses ANSI SGR StylePair constants from `@cap/ansi-styles` with zero duplication.

import { strEq, startsWith } from "cap:ascii";
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
    FG_RED_BRIGHT,
    FG_GREEN_BRIGHT,
    FG_YELLOW_BRIGHT,
    FG_BLUE_BRIGHT,
    FG_MAGENTA_BRIGHT,
    FG_CYAN_BRIGHT,
    FG_WHITE_BRIGHT,
    BG_BLACK,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE,
    BG_GRAY,
    BG_CLOSE
} from "../../ansi-styles/src/index.cp";

const CLR_BG_RED_BRIGHT: StylePair = StylePair { open: "\x1b[101m", close: BG_CLOSE };
const CLR_BG_GREEN_BRIGHT: StylePair = StylePair { open: "\x1b[102m", close: BG_CLOSE };
const CLR_BG_YELLOW_BRIGHT: StylePair = StylePair { open: "\x1b[103m", close: BG_CLOSE };
const CLR_BG_BLUE_BRIGHT: StylePair = StylePair { open: "\x1b[104m", close: BG_CLOSE };
const CLR_BG_MAGENTA_BRIGHT: StylePair = StylePair { open: "\x1b[105m", close: BG_CLOSE };
const CLR_BG_CYAN_BRIGHT: StylePair = StylePair { open: "\x1b[106m", close: BG_CLOSE };
const CLR_BG_WHITE_BRIGHT: StylePair = StylePair { open: "\x1b[107m", close: BG_CLOSE };

// Formats `text` with `pair`, replacing embedded `pair.close` sequences with `replace_with`
// to restore outer styles when nested. Passes `text` through verbatim when `use_color == false`.
fn clrFormatPair(
    use_color: bool,
    pair: StylePair,
    replace_with: string,
    text: string,
    out: []u8
): usize {
    if (text.len == 0 || out.len == 0) {
        return 0;
    }
    if (!use_color) {
        return writeStr(text, out, 0);
    }

    let p: usize = 0;
    p = writeStr(pair.open, out, p);

    let i: usize = 0;
    while (i < text.len && p < out.len) {
        if (pair.close.len > 0 && startsWith(text[i..text.len], pair.close)) {
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

/// Detects terminal color support following `colorette` v2 environment rules:
/// - Non-empty `NO_COLOR` disables color output.
/// - Non-empty `FORCE_COLOR` enables color output unless `"0"`.
/// - `TERM == "dumb"` disables color output.
/// - Otherwise falls back to `is_tty`.
pub fn isColorSupported(no_color: string, force_color: string, term: string, is_tty: bool): bool {
    if (no_color.len > 0) {
        return false;
    }
    if (force_color.len > 0) {
        if (strEq(force_color, "0")) {
            return false;
        }
        return true;
    }
    if (strEq(term, "dumb")) {
        return false;
    }
    return is_tty;
}

/// Configuration options for creating a `Colorette` formatter instance.
pub struct ColoretteOptions {
    pub use_color: bool,

    pub static fn enabled(): ColoretteOptions {
        return ColoretteOptions { use_color: true };
    }

    pub static fn disabled(): ColoretteOptions {
        return ColoretteOptions { use_color: false };
    }

    pub static fn fromEnv(no_color: string, force_color: string, term: string, is_tty: bool): ColoretteOptions {
        return ColoretteOptions {
            use_color: isColorSupported(no_color, force_color, term, is_tty)
        };
    }
}

/// Zero-heap ANSI color formatter writing directly into caller-provided `[]u8` buffers.
pub struct Colorette {
    pub options: ColoretteOptions,

    pub static fn new(use_color: bool): Colorette {
        return Colorette {
            options: ColoretteOptions { use_color: use_color }
        };
    }

    pub static fn fromOptions(opts: ColoretteOptions): Colorette {
        return Colorette { options: opts };
    }

    pub fn isEnabled(): bool {
        return _.options.use_color;
    }

    pub fn formatPair(pair: StylePair, text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, pair, pair.open, text, out);
    }

    // Modifiers
    pub fn reset(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, RESET, RESET.open, text, out);
    }

    pub fn bold(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BOLD, "\x1b[22m\x1b[1m", text, out);
    }

    pub fn dim(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, DIM, "\x1b[22m\x1b[2m", text, out);
    }

    pub fn italic(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, ITALIC, ITALIC.open, text, out);
    }

    pub fn underline(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, UNDERLINE, UNDERLINE.open, text, out);
    }

    pub fn inverse(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, INVERSE, INVERSE.open, text, out);
    }

    pub fn hidden(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, HIDDEN, HIDDEN.open, text, out);
    }

    pub fn strikethrough(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, STRIKETHROUGH, STRIKETHROUGH.open, text, out);
    }

    // Standard foreground colors
    pub fn black(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_BLACK, FG_BLACK.open, text, out);
    }

    pub fn red(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_RED, FG_RED.open, text, out);
    }

    pub fn green(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_GREEN, FG_GREEN.open, text, out);
    }

    pub fn yellow(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_YELLOW, FG_YELLOW.open, text, out);
    }

    pub fn blue(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_BLUE, FG_BLUE.open, text, out);
    }

    pub fn magenta(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_MAGENTA, FG_MAGENTA.open, text, out);
    }

    pub fn cyan(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_CYAN, FG_CYAN.open, text, out);
    }

    pub fn white(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_WHITE, FG_WHITE.open, text, out);
    }

    pub fn gray(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_GRAY, FG_GRAY.open, text, out);
    }

    // Bright foreground colors
    pub fn blackBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_GRAY, FG_GRAY.open, text, out);
    }

    pub fn redBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_RED_BRIGHT, FG_RED_BRIGHT.open, text, out);
    }

    pub fn greenBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_GREEN_BRIGHT, FG_GREEN_BRIGHT.open, text, out);
    }

    pub fn yellowBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_YELLOW_BRIGHT, FG_YELLOW_BRIGHT.open, text, out);
    }

    pub fn blueBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_BLUE_BRIGHT, FG_BLUE_BRIGHT.open, text, out);
    }

    pub fn magentaBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_MAGENTA_BRIGHT, FG_MAGENTA_BRIGHT.open, text, out);
    }

    pub fn cyanBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_CYAN_BRIGHT, FG_CYAN_BRIGHT.open, text, out);
    }

    pub fn whiteBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, FG_WHITE_BRIGHT, FG_WHITE_BRIGHT.open, text, out);
    }

    // Standard background colors
    pub fn bgBlack(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_BLACK, BG_BLACK.open, text, out);
    }

    pub fn bgRed(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_RED, BG_RED.open, text, out);
    }

    pub fn bgGreen(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_GREEN, BG_GREEN.open, text, out);
    }

    pub fn bgYellow(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_YELLOW, BG_YELLOW.open, text, out);
    }

    pub fn bgBlue(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_BLUE, BG_BLUE.open, text, out);
    }

    pub fn bgMagenta(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_MAGENTA, BG_MAGENTA.open, text, out);
    }

    pub fn bgCyan(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_CYAN, BG_CYAN.open, text, out);
    }

    pub fn bgWhite(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_WHITE, BG_WHITE.open, text, out);
    }

    pub fn bgGray(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_GRAY, BG_GRAY.open, text, out);
    }

    // Bright background colors
    pub fn bgBlackBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, BG_GRAY, BG_GRAY.open, text, out);
    }

    pub fn bgRedBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_RED_BRIGHT, CLR_BG_RED_BRIGHT.open, text, out);
    }

    pub fn bgGreenBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_GREEN_BRIGHT, CLR_BG_GREEN_BRIGHT.open, text, out);
    }

    pub fn bgYellowBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_YELLOW_BRIGHT, CLR_BG_YELLOW_BRIGHT.open, text, out);
    }

    pub fn bgBlueBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_BLUE_BRIGHT, CLR_BG_BLUE_BRIGHT.open, text, out);
    }

    pub fn bgMagentaBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_MAGENTA_BRIGHT, CLR_BG_MAGENTA_BRIGHT.open, text, out);
    }

    pub fn bgCyanBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_CYAN_BRIGHT, CLR_BG_CYAN_BRIGHT.open, text, out);
    }

    pub fn bgWhiteBright(text: string, out: []u8): usize {
        return clrFormatPair(_.options.use_color, CLR_BG_WHITE_BRIGHT, CLR_BG_WHITE_BRIGHT.open, text, out);
    }
}

/// Factory function matching `colorette`'s `createColors({ useColor })` API.
pub fn createColors(use_color: bool): Colorette {
    return Colorette.new(use_color);
}

/// Factory function accepting a `ColoretteOptions` struct.
pub fn createColorsWithOptions(opts: ColoretteOptions): Colorette {
    return Colorette.fromOptions(opts);
}

export {
    isColorSupported,
    ColoretteOptions,
    Colorette,
    createColors,
    createColorsWithOptions
};
