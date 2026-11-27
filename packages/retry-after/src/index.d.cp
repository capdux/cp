// Type definitions for CP

import { trim } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { parseU64, writeStr, writeU64 } from "cap:fmt";
export class RetryAfter {
    kind: u8,
    delay_seconds: u64,
    epoch_seconds: i64,
    valid: bool,
}

export fn parseUint(s: &[]u8): i64;
export fn parseMonth(s: &[]u8): i32;
export fn isLeapYear(y: i64): bool;
export fn daysInMonth(y: i64, m: i32): i32;
export fn daysFromCivil(y_in: i64, m_in: i64, d: i64): i64;
export fn parseHttpDate(s: &[]u8): i64;
export fn parseRetryAfter(header: &[]u8): RetryAfter;
export fn waitSeconds(ra: RetryAfter, now_epoch_sec: i64): i64;
export fn formatDelaySeconds(seconds: u64, dst: []u8): i64;
