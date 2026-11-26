// Type definitions for CP

export class ProgressBar {
    total: u64,
    current: u64,
    width: usize,
    complete_char: u8,
    incomplete_char: u8,
    head_char: u8,
    elapsed_ms: u64,
    fn ratio(): f64;
    fn percent(): u32;
    fn isComplete(): bool;
    fn tick(_: auto, delta: u64);
    fn etaSeconds(): f64;
}

export fn newProgressBar(total: u64, width: usize): ProgressBar;
export fn prgWriteStr(s: string, out: []u8, pos: usize): usize;
export fn prgWriteU64(val: u64, out: []u8, pos: usize): usize;
export fn prgStartsWithAt(s: string, offset: usize, prefix: string): bool;
export fn prgWriteBar(bar: ProgressBar, out: []u8, pos: usize): usize;
export fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize;
