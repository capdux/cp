// Type definitions for CP

import { strEq, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { StylePair, RESET, BOLD, DIM, ITALIC, UNDERLINE, INVERSE, HIDDEN, STRIKETHROUGH, FG_BLACK, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE, FG_MAGENTA, FG_CYAN, FG_WHITE, FG_GRAY, BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW, BG_BLUE, BG_MAGENTA, BG_CYAN, BG_WHITE } from "../../ansi-styles/src/index.cp";
export class KleurChain {
    enabled: bool,
    stack: [8]StylePair,
    depth: usize,
    fn new(enabled: bool): KleurChain;
    fn on(): KleurChain;
    fn off(): KleurChain;
    fn depthCount(): usize;
    fn withStyle(pair: StylePair): KleurChain;
    fn reset(): KleurChain;
    fn bold(): KleurChain;
    fn dim(): KleurChain;
    fn italic(): KleurChain;
    fn underline(): KleurChain;
    fn inverse(): KleurChain;
    fn hidden(): KleurChain;
    fn strikethrough(): KleurChain;
    fn black(): KleurChain;
    fn red(): KleurChain;
    fn green(): KleurChain;
    fn yellow(): KleurChain;
    fn blue(): KleurChain;
    fn magenta(): KleurChain;
    fn cyan(): KleurChain;
    fn white(): KleurChain;
    fn gray(): KleurChain;
    fn bgBlack(): KleurChain;
    fn bgRed(): KleurChain;
    fn bgGreen(): KleurChain;
    fn bgYellow(): KleurChain;
    fn bgBlue(): KleurChain;
    fn bgMagenta(): KleurChain;
    fn bgCyan(): KleurChain;
    fn bgWhite(): KleurChain;
    fn format(text: string, out: []u8): usize;
}

