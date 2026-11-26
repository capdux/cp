// Type definitions for CP

export class Utf8Step {
    cp: u32,
    width: usize,
}

export fn decodeUtf8At(s: string, i: usize): Utf8Step;
export fn codepointWidth(cp: u32): usize;
export fn scanAnsiCsi(s: string, i: usize): usize;
export fn isResetSgr(seq: string): bool;
export fn startsWithZeroReset(seq: string): bool;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export class AnsiStyleState {
    seqs: [MAX_ACTIVE_SGR]string,
    count: usize,
    fn empty(): AnsiStyleState;
    fn hasActive(): bool;
    fn applySeq(_: auto, seq: string);
    fn emitActive(out: []u8, pos: usize): usize;
}

export fn visibleWidth(s: string): usize;
export fn emitLineBreak(out: []u8, pos: usize, style: AnsiStyleState): usize;
export fn wrapAnsi(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string;
export fn sliceAnsi(text: string, start_col: usize, end_col: usize, out: []u8): string;
