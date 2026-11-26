// Type definitions for CP

import { hasAnsi, stripAnsi } from "../../ansi-strip/src/index.cp";
import { charWidth, charWidthCjk } from "../../unicode-width/src/index.cp";
export class StringWidthOptions {
    ambiguous_is_narrow: bool,
    count_ansi_escape_codes: bool,
    fn defaults(): StringWidthOptions;
}

export class SwDecoded {
    cp: u32,
    next_pos: usize,
}

export fn swDecodeUtf8(s: string, pos: usize): SwDecoded;
export fn swIsRegionalIndicator(cp: u32): bool;
export fn swIsSkinToneModifier(cp: u32): bool;
export fn swIsTagChar(cp: u32): bool;
export fn swBaseCharWidth(cp: u32, ambiguous_is_narrow: bool): usize;
export fn swMeasurePlain(s: string, ambiguous_is_narrow: bool): usize;
export fn swMeasureStrippedChunk(chunk: string, ambiguous_is_narrow: bool): usize;
export fn stringWidthWithOptions(input: string, opts: StringWidthOptions): usize;
export fn stringWidth(input: string): usize;
export fn maxLineWidthWithOptions(multiline: string, opts: StringWidthOptions): usize;
export fn maxLineWidth(multiline: string): usize;
export fn swCopyBytes(dst: []u8, pos: usize, src: string): usize;
export fn swFillBytes(dst: []u8, pos: usize, ch: u8, count: usize): usize;
export fn padEndVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;
export fn padStartVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;
