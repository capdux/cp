// Type definitions for CP

import { charWidth } from "../../unicode-width/src/index.cp";
import { isSpace, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class WrapOptions {
    width: usize,
    break_words: bool,
    preserve_newlines: bool,
    fn defaults(): WrapOptions;
    fn withWidth(width: usize): WrapOptions;
}

export class DecodedChar {
    cp: u32,
    len: usize,
}

export class WrapCursor {
    pos: usize,
    col: usize,
}

export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn decodeUtf8(s: string, pos: usize): DecodedChar;
export fn ansiSeqLen(s: string, pos: usize): usize;
export fn isAnsiReset(s: string, pos: usize, seq_len: usize): bool;
export fn codepointWidth(cp: u32): usize;
export fn isSpaceByte(c: u8): bool;
export fn displayWidth(s: string): usize;
export fn stripAnsi(s: string, out: []u8): string;
export fn writeHardBrokenToken(text: string, tok_start: usize, tok_end: usize, width: usize, out: []u8, start_pos: usize, start_col: usize): WrapCursor;
export fn wrapSegment(text: string, seg_start: usize, seg_end: usize, opts: WrapOptions, out: []u8, start_pos: usize): usize;
export fn wrapTextWith(text: string, opts: WrapOptions, out: []u8): string;
export fn wrapText(text: string, width: usize, out: []u8): string;
export fn lineHasContent(text: string, start: usize, end: usize): bool;
export fn indent(text: string, prefix: string, out: []u8): string;
export fn dedent(text: string, out: []u8): string;
export fn truncate(text: string, max_width: usize, ellipsis: string, out: []u8): string;
