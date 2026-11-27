// Type definitions for CP

import { startsWith } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
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
export fn prgWriteBar(bar: ProgressBar, out: []u8, pos: usize): usize;
export fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize;
