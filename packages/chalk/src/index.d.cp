// Type definitions for CP

import { StylePair, BOLD, DIM, ITALIC, UNDERLINE, INVERSE, STRIKETHROUGH, FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE, FG_GRAY, BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE, BG_GRAY, FG_CLOSE, BG_CLOSE, wrapStyle, formatAnsi256, formatAnsi16m, formatRgbToAnsi16, formatRgbToAnsi256, formatHexToAnsi16m, formatKeywordToAnsi16m } from "../../ansi-styles/src/index.cp";
import { hexToRgb, keywordToRgb } from "../../color-convert/src/index.cp";
import { COLOR_LEVEL_NONE, COLOR_LEVEL_BASIC, COLOR_LEVEL_256, COLOR_LEVEL_16M, TerminalEnv, detectColorLevel } from "../../supports-color/src/index.cp";
import { writeStr } from "cap:fmt";
export fn chkCopyPlain(text: string, out: []u8): usize;
export fn chkWrapOpenClose(open: string, text: string, close: string, out: []u8): usize;
export fn chkApplyPair(level: u8, style: StylePair, text: string, out: []u8): usize;
export class Chalk {
    level: u8,
    fn new(level: u8): Chalk;
    fn fromEnv(env: TerminalEnv, argv: &[]string): Chalk;
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
    fn bgGray(text: string, out: []u8): usize;
    fn bold(text: string, out: []u8): usize;
    fn dim(text: string, out: []u8): usize;
    fn italic(text: string, out: []u8): usize;
    fn underline(text: string, out: []u8): usize;
    fn inverse(text: string, out: []u8): usize;
    fn strikethrough(text: string, out: []u8): usize;
    fn ansi256(code: u8, text: string, out: []u8): usize;
    fn bgAnsi256(code: u8, text: string, out: []u8): usize;
    fn rgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize;
    fn bgRgb(r: u8, g: u8, b: u8, text: string, out: []u8): usize;
    fn hex(hex_str: string, text: string, out: []u8): usize;
    fn bgHex(hex_str: string, text: string, out: []u8): usize;
    fn keyword(name: string, text: string, out: []u8): usize;
    fn bgKeyword(name: string, text: string, out: []u8): usize;
    fn styled(text: string, is_bold: bool, is_underline: bool, fg: StylePair, out: []u8): usize;
}

