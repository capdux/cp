// Type definitions for CP

import { prettyBytes } from "../../pretty-bytes/src/index.cp";
import { prettyMs } from "../../pretty-ms/src/index.cp";
export class ProgressState {
    pos: u64,
    total: u64,
    elapsed_ms: u64,
    width: usize,
}

export fn indWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn indWriteStr(out: []u8, pos: usize, s: &[]u8): usize;
export fn indWriteU64(out: []u8, pos: usize, val: u64): usize;
export fn percent(s: ProgressState): f64;
export fn etaMs(s: ProgressState): u64;
export fn bytesPerSec(s: ProgressState): f64;
export fn renderBar(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize;
export fn renderLine(s: ProgressState, prefix: string, out: []u8): usize;
