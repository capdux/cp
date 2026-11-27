// Type definitions for CP

import { stringWidth } from "../../string-width/src/index.cp";
import { sliceAnsi, sliceAnsiFrom } from "../../slice-ansi/src/index.cp";
export enum TruncatePosition {
    END = 0,
    START = 1,
    MIDDLE = 2
}

export class TruncateOptions {
    position: TruncatePosition,
    prefer_truncation_on_space: bool,
    ellipsis: string,
    fn defaults(): TruncateOptions;
}

export fn ctWriteBytes(out: []u8, pos: usize, src: string): usize;
export fn ctSkipAnsi(text: string, pos: usize): usize;
export fn ctUtf8Len(text: string, pos: usize): usize;
export fn ctFindLastSpaceLe(text: string, max_col: usize): usize;
export fn ctIsSpaceAtCol(text: string, target_col: usize): bool;
export fn ctFindFirstSpaceGe(text: string, min_col: usize, limit_col: usize): usize;
export fn cliTruncateInto(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize;
export fn cliTruncateEndInto(text: string, columns: usize, out: []u8): usize;
export fn cliTruncateStartInto(text: string, columns: usize, out: []u8): usize;
export fn cliTruncateMiddleInto(text: string, columns: usize, out: []u8): usize;
