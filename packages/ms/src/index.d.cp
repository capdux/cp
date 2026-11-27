// Type definitions for CP

import { formatI64 } from "../../itoa/src/index.cp";
export class MsParseResult {
    ok: bool,
    ms: i64,
}

export fn msIsSpace(c: u8): bool;
export fn msIsDigit(c: u8): bool;
export fn msEqIgnoreCase(s: string, start: usize, end: usize, lit: string): bool;
export fn msUnitMultiplier(s: string, start: usize, end: usize): f64;
export fn parseMs(input: string): MsParseResult;
export fn msPushStr(out: []u8, pos: usize, s: string): usize;
export fn msRoundDiv(ms: i64, unit: i64): i64;
export fn msFormatUnit(val: i64, short_unit: string, long_singular: string, long_plural: string, long_fmt: bool, out: []u8): usize;
export fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize;
