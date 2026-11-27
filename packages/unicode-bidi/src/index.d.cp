// Type definitions for CP

export class BidiRun {
    start: usize,
    len: usize,
    level: u8,
}

export class Utf8Step {
    cp: u32,
    len: usize,
}

export fn decodeUtf8At(text: string, pos: usize): Utf8Step;
export fn bidiClass(cp: u32): u8;
export fn baseDirection(text: string): u8;
export fn hasRtl(text: string): bool;
export fn isLtrOnly(text: string): bool;
export fn resolveCharLevel(cls: u8, base_level: u8, prev_level: u8): u8;
export fn computeVisualRuns(text: string, out_runs: []BidiRun): usize;
export fn mirrorAsciiBracket(ch: u8): u8;
