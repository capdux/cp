// Type definitions for CP

import { stringWidth } from "../../string-width/src/index.cp";
import { widestLine } from "../../widest-line/src/index.cp";

export enum AlignMode {
    Left,
    Center,
    Right
}

export class AlignOptions {
    mode: AlignMode,
    split: string,
    pad: string,
    fn default(): AlignOptions;
}

export fn ansiVisibleWidth(s: string): usize;
export fn ansiMaxLineWidth(text: string, split: string): usize;
export fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize;
export fn alignAnsiWithOptions(text: string, opts: AlignOptions, out: []u8): usize;
export fn alignAnsiLines(lines: []const string, mode: AlignMode, pad: string, split: string, out: []u8): usize;
export fn alignCenter(text: string, out: []u8): usize;
export fn alignRight(text: string, out: []u8): usize;
export fn alignLeft(text: string, out: []u8): usize;
