// Type definitions for CP

import { formatU64 } from "../../itoa/src/index.cp";
export class DurationParseResult {
    ok: bool,
    nanos: u64,
}

export fn htIsSpace(c: u8): bool;
export fn htIsDigit(c: u8): bool;
export fn htIsAlpha(c: u8): bool;
export fn htEq(s: string, start: usize, end: usize, lit: string): bool;
export fn htUnitNanos(s: string, start: usize, end: usize): u64;
export fn parseDuration(input: string): DurationParseResult;
export fn htPushStr(out: []u8, pos: usize, s: string): usize;
export fn htPushPart(out: []u8, pos: usize, val: u64, suffix: string): usize;
export fn formatDuration(nanos: u64, out: []u8): usize;
export fn htPush2Digits(out: []u8, pos: usize, v: u64): usize;
export fn htPush4Digits(out: []u8, pos: usize, v: u64): usize;
export fn formatRfc3339Seconds(unix_secs: u64, out: []u8): usize;
