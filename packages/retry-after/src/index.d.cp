// Type definitions for CP

export class RetryAfter {
    kind: u8,
    delay_seconds: u64,
    epoch_seconds: i64,
    valid: bool,
}

export fn trimSpaces(s: &[]u8): &[]u8;
export fn parseUint(s: &[]u8): i64;
export fn eq3(s: &[]u8, a: u8, b: u8, c: u8): bool;
export fn parseMonth(s: &[]u8): i32;
export fn isLeapYear(y: i64): bool;
export fn daysInMonth(y: i64, m: i32): i32;
export fn daysFromCivil(y_in: i64, m_in: i64, d: i64): i64;
export fn parseHttpDate(s: &[]u8): i64;
export fn parseRetryAfter(header: &[]u8): RetryAfter;
export fn waitSeconds(ra: RetryAfter, now_epoch_sec: i64): i64;
export fn formatDelaySeconds(seconds: u64, dst: []u8): i64;
