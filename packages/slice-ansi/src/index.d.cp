// Type definitions for CP

import { TOKEN_TEXT, TOKEN_CSI, AnsiToken, SgrParams, tokenizeAnsi, parseCsiParams } from "../../anstyle-parse/src/index.cp";
import { charWidth, strWidth } from "../../unicode-width/src/index.cp";
export class SaUtf8Char {
    cp: u32,
    byte_len: usize,
}

export class SaStyleState {
    bold: bool,
    dim: bool,
    italic: bool,
    underline: bool,
    blink: bool,
    inverse: bool,
    hidden: bool,
    strikethrough: bool,
    fg_mode: u8,
    fg_code: u16,
    fg_r: u8,
    fg_g: u8,
    fg_b: u8,
    bg_mode: u8,
    bg_code: u16,
    bg_r: u8,
    bg_g: u8,
    bg_b: u8,
    fn empty(): SaStyleState;
    fn reset(_: auto);
    fn hasAttr(): bool;
    fn hasAny(): bool;
    fn applySgr(_: auto, seq: string);
}

export fn saDecodeUtf8(s: string, pos: usize): SaUtf8Char;
export fn saWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn saWriteBytes(out: []u8, pos: usize, src: string): usize;
export fn saWriteU16(out: []u8, pos: usize, val: u16): usize;
export fn saWriteSgrParam(out: []u8, pos: usize, first: &mut bool, code: u16): usize;
export fn saEmitActiveStyle(out: []u8, pos: usize, st: SaStyleState): usize;
export fn saEmitCloseStyle(out: []u8, pos: usize, st: SaStyleState): usize;
export fn ansiVisibleWidth(input: string): usize;
export fn sliceAnsi(input: string, begin: usize, end: usize, out: []u8): usize;
export fn sliceAnsiFrom(input: string, begin: usize, out: []u8): usize;
export fn truncateAnsi(input: string, max_cols: usize, ellipsis: string, out: []u8): usize;
