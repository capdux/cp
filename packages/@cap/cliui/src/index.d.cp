// Type definitions for CP

import { wrapAnsi, visibleWidth } from "../../wrap-ansi/src/index.cp";
import { hasAnsi, stripAnsi } from "../../ansi-strip/src/index.cp";
import { strWidth } from "../../unicode-width/src/index.cp";
export enum Align {
    Left,
    Right,
    Center
}

export fn alignToU8(a: Align): u8;
export class Column {
    text: string,
    width: usize,
    alignment: u8,
    pad_left: usize,
    pad_right: usize,
    border: bool,
    fn plain(text: string): Column;
    fn sized(text: string, width: usize): Column;
}

export fn cuWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn cuWriteBytes(out: []u8, pos: usize, s: string): usize;
export fn cuWriteSpaces(out: []u8, pos: usize, count: usize): usize;
export fn cuVisibleWidth(s: string): usize;
export fn cuCountLines(s: string): usize;
export fn cuGetLine(s: string, target_line: usize): string;
export fn cuResolveWidths(total_width: usize, cols: &[]Column, n_cols: usize, widths: []usize);
export class CliUi {
    width: usize,
    len: usize,
    row_count: usize,
    in_span: bool,
    buf: [CU_BUF_CAP]u8,
    fn new(width: usize): CliUi;
    fn resetOutput(_: auto);
    fn div(_: auto, cols: &[]Column);
    fn span(_: auto, cols: &[]Column);
    fn render(out: []u8): usize;
}

export fn formatTwoCol(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize;
