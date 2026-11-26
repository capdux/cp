// Type definitions for CP

import { stringWidth, maxLineWidth } from "../../string-width/src/index.cp";
export class BoxenOptions {
    border_style: u8,
    padding_x: usize,
    padding_y: usize,
    text_align: u8,
    title: string,
    fn defaultOpts(): BoxenOptions;
}

export class BxnBorder {
    tl: string,
    horiz: string,
    tr: string,
    vert: string,
    bl: string,
    br: string,
}

export fn bxnGetBorder(style: u8): BxnBorder;
export fn bxnWriteByte(dst: []u8, pos: usize, ch: u8): usize;
export fn bxnWriteStr(dst: []u8, pos: usize, s: string): usize;
export fn bxnRepeatByte(dst: []u8, pos: usize, ch: u8, count: usize): usize;
export fn bxnRepeatStr(dst: []u8, pos: usize, s: string, count: usize): usize;
export fn bxnWriteEmptyRow(dst: []u8, pos: usize, border: BxnBorder, inner_w: usize): usize;
export fn bxnWriteContentLine(dst: []u8, pos: usize, border: BxnBorder, line: string, inner_w: usize, padding_x: usize, text_align: u8): usize;
export fn renderBox(text: string, opts: BoxenOptions, out: []u8): usize;
