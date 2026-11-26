// Type definitions for CP

import { DIFF_EQUAL, DIFF_DELETE, DIFF_INSERT, DiffOp, diffLines, hasChanges } from "../../similar-diff/src/index.cp";
import { PicoFormatter } from "../../picocolors/src/index.cp";
export class JdfLine {
    start: usize,
    end: usize,
}

export fn jdfSplitLines(text: string, out_lines: []JdfLine): usize;
export fn jdfWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize;
export fn jdfWriteStr(out: []u8, pos: usize, s: string): usize;
export fn jdfWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn jdfWriteUsize(out: []u8, pos: usize, val: usize): usize;
export fn jdfEmitStyledLine(fmt: PicoFormatter, color_kind: u8, line_text: string, out: []u8, pos: usize): usize;
export class DiffOptions {
    a_annotation: string,
    b_annotation: string,
    context_lines: usize,
    expand: bool,
    include_change_counts: bool,
    use_color: bool,
    fn defaults(): DiffOptions;
    fn colored(): DiffOptions;
}

export class DiffSummary {
    insertions: usize,
    deletions: usize,
    equal: usize,
    has_diff: bool,
}

export fn diffStats(expected: string, received: string): DiffSummary;
export fn diffLinesUnified(expected: string, received: string, opts: DiffOptions, out: []u8): usize;
