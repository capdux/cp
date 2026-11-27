// Type definitions for CP

import { stringWidth } from "../../string-width/src/index.cp";
import { widestLine } from "../../widest-line/src/index.cp";
export enum AlignMode {
    Left,
    Center,
    Right
}

export fn aaWriteSpaces(out: []u8, pos: usize, count: usize): usize;
export fn aaCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;
export fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize;
export fn alignCenter(text: string, out: []u8): usize;
export fn alignRight(text: string, out: []u8): usize;
export fn alignLeft(text: string, out: []u8): usize;
