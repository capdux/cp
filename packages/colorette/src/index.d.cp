// Type definitions for CP

import { strEq, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { StylePair, RESET, BOLD, DIM, ITALIC, UNDERLINE, INVERSE, HIDDEN, STRIKETHROUGH, FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE, FG_GRAY, FG_RED_BRIGHT, FG_GREEN_BRIGHT, FG_YELLOW_BRIGHT, FG_BLUE_BRIGHT, FG_MAGENTA_BRIGHT, FG_CYAN_BRIGHT, FG_WHITE_BRIGHT, BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE, BG_GRAY, BG_CLOSE } from "../../ansi-styles/src/index.cp";
export fn clrFormatPair(use_color: bool, pair: StylePair, replace_with: string, text: string, out: []u8): usize;
export fn isColorSupported(no_color: string, force_color: string, term: string, is_tty: bool): bool;
export class ColoretteOptions {
    use_color: bool,
    fn enabled(): ColoretteOptions;
    fn disabled(): ColoretteOptions;
    fn fromEnv(no_color: string, force_color: string, term: string, is_tty: bool): ColoretteOptions;
}

export class Colorette {
    options: ColoretteOptions,
    fn new(use_color: bool): Colorette;
    fn fromOptions(opts: ColoretteOptions): Colorette;
    fn isEnabled(): bool;
    fn formatPair(pair: StylePair, text: string, out: []u8): usize;
    fn reset(text: string, out: []u8): usize;
    fn bold(text: string, out: []u8): usize;
    fn dim(text: string, out: []u8): usize;
    fn italic(text: string, out: []u8): usize;
    fn underline(text: string, out: []u8): usize;
    fn inverse(text: string, out: []u8): usize;
    fn hidden(text: string, out: []u8): usize;
    fn strikethrough(text: string, out: []u8): usize;
    fn black(text: string, out: []u8): usize;
    fn red(text: string, out: []u8): usize;
    fn green(text: string, out: []u8): usize;
    fn yellow(text: string, out: []u8): usize;
    fn blue(text: string, out: []u8): usize;
    fn magenta(text: string, out: []u8): usize;
    fn cyan(text: string, out: []u8): usize;
    fn white(text: string, out: []u8): usize;
    fn gray(text: string, out: []u8): usize;
    fn blackBright(text: string, out: []u8): usize;
    fn redBright(text: string, out: []u8): usize;
    fn greenBright(text: string, out: []u8): usize;
    fn yellowBright(text: string, out: []u8): usize;
    fn blueBright(text: string, out: []u8): usize;
    fn magentaBright(text: string, out: []u8): usize;
    fn cyanBright(text: string, out: []u8): usize;
    fn whiteBright(text: string, out: []u8): usize;
    fn bgBlack(text: string, out: []u8): usize;
    fn bgRed(text: string, out: []u8): usize;
    fn bgGreen(text: string, out: []u8): usize;
    fn bgYellow(text: string, out: []u8): usize;
    fn bgBlue(text: string, out: []u8): usize;
    fn bgMagenta(text: string, out: []u8): usize;
    fn bgCyan(text: string, out: []u8): usize;
    fn bgWhite(text: string, out: []u8): usize;
    fn bgGray(text: string, out: []u8): usize;
    fn bgBlackBright(text: string, out: []u8): usize;
    fn bgRedBright(text: string, out: []u8): usize;
    fn bgGreenBright(text: string, out: []u8): usize;
    fn bgYellowBright(text: string, out: []u8): usize;
    fn bgBlueBright(text: string, out: []u8): usize;
    fn bgMagentaBright(text: string, out: []u8): usize;
    fn bgCyanBright(text: string, out: []u8): usize;
    fn bgWhiteBright(text: string, out: []u8): usize;
}

export fn createColors(use_color: bool): Colorette;
export fn createColorsWithOptions(opts: ColoretteOptions): Colorette;
