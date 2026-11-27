// Type definitions for CP

import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { StylePair, RESET, BOLD, DIM, ITALIC, UNDERLINE, INVERSE, HIDDEN, STRIKETHROUGH, FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE, FG_GRAY, BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE } from "../../ansi-styles/src/index.cp";
export fn picoMatchAt(haystack: string, offset: usize, needle: string): bool;
export fn picoFormatPair(enabled: bool, pair: StylePair, replace_with: string, text: string, out: []u8): usize;
export fn detectColorSupport(no_color: string, force_color: string, is_tty: bool): bool;
export class ColorConfig {
    is_color_supported: bool,
    fn enabled(): ColorConfig;
    fn disabled(): ColorConfig;
    fn fromEnv(no_color: string, force_color: string, is_tty: bool): ColorConfig;
}

export class PicoFormatter {
    cfg: ColorConfig,
    fn new(is_color_supported: bool): PicoFormatter;
    fn fromConfig(cfg: ColorConfig): PicoFormatter;
    fn isColorSupported(): bool;
    fn style(pair: StylePair, text: string, out: []u8): usize;
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
    fn bgBlack(text: string, out: []u8): usize;
    fn bgRed(text: string, out: []u8): usize;
    fn bgGreen(text: string, out: []u8): usize;
    fn bgYellow(text: string, out: []u8): usize;
    fn bgBlue(text: string, out: []u8): usize;
    fn bgMagenta(text: string, out: []u8): usize;
    fn bgCyan(text: string, out: []u8): usize;
    fn bgWhite(text: string, out: []u8): usize;
}

export class PicoBuf {
    cfg: ColorConfig,
    buf: [1024]u8,
    len: usize,
    fn new(is_color_supported: bool): PicoBuf;
    fn fromConfig(cfg: ColorConfig): PicoBuf;
    fn clear(_: auto);
    fn pushPlain(_: auto, text: string): usize;
    fn pushStyled(_: auto, pair: StylePair, text: string): usize;
    fn bold(_: auto, text: string): usize;
    fn dim(_: auto, text: string): usize;
    fn italic(_: auto, text: string): usize;
    fn underline(_: auto, text: string): usize;
    fn inverse(_: auto, text: string): usize;
    fn strikethrough(_: auto, text: string): usize;
    fn black(_: auto, text: string): usize;
    fn red(_: auto, text: string): usize;
    fn green(_: auto, text: string): usize;
    fn yellow(_: auto, text: string): usize;
    fn blue(_: auto, text: string): usize;
    fn magenta(_: auto, text: string): usize;
    fn cyan(_: auto, text: string): usize;
    fn white(_: auto, text: string): usize;
    fn gray(_: auto, text: string): usize;
    fn bgRed(_: auto, text: string): usize;
    fn bgGreen(_: auto, text: string): usize;
    fn bgYellow(_: auto, text: string): usize;
    fn bgBlue(_: auto, text: string): usize;
}

