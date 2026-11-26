// Type definitions for CP

import { strWidth } from "../../unicode-width/src/index.cp";
export fn twpIsInlineSpace(b: u8): bool;
export fn twpWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn twpWriteSlice(out: []u8, pos: usize, src: string): usize;
export fn twpLineContentEnd(text: string, start: usize, nl_pos: usize): usize;
export fn twpFindNewline(text: string, start: usize): usize;
export fn twpSkipInlineSpaces(text: string, start: usize, end: usize): usize;
export fn twpTrimRightInlineSpaces(text: string, start: usize, end: usize): usize;
export fn twpIsBlankLine(text: string, start: usize, end: usize): bool;
export fn lineDisplayWidth(line: string): usize;
export fn fillWithIndent(text: string, width: usize, initial_indent: string, subsequent_indent: string, out: []u8): usize;
export fn fill(text: string, width: usize, out: []u8): usize;
export fn indent(text: string, prefix: string, out: []u8): usize;
export fn dedent(text: string, out: []u8): usize;
export fn unfill(text: string, out: []u8): usize;
