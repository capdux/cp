// Type definitions for CP

import { ParsedMs, parseMs } from "../../parse-ms/src/index.cp";
export class PrettyMsOptions {
    verbose: bool,
    compact: bool,
    colon_notation: bool,
    separate_ms: bool,
    unit_count: u8,
    fn defaults(): PrettyMsOptions;
}

export fn prettyAbsI64(v: i64): u64;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn pushU64(out: []u8, pos: usize, val: u64): usize;
export fn pushTwoDigits(out: []u8, pos: usize, val: u8): usize;
export fn emitUnit(out: []u8, pos: usize, emitted: u8, val: u64, short_unit: string, singular: string, plural: string, verbose: bool): usize;
export fn prettyMs(ms: i64, out: []u8): string;
export fn prettyMsWith(ms: i64, opts: PrettyMsOptions, out: []u8): string;
