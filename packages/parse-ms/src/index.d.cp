// Type definitions for CP

export const MS_PER_SECOND: u64;
export const MS_PER_MINUTE: u64;
export const MS_PER_HOUR: u64;
export const MS_PER_DAY: u64;

export class ParsedMs {
    days: i64,
    hours: i64,
    minutes: i64,
    seconds: i64,
    milliseconds: i64,
    microseconds: i64,
    nanoseconds: i64,
    fn zero(): ParsedMs;
    fn isZero(): bool;
    fn isNegative(): bool;
    fn equals(other: ParsedMs): bool;
}

export fn parseMs(ms: i64): ParsedMs;
export fn parseMsF64Checked(ms: f64): ParsedMs?;
export fn parseMsF64(ms: f64): ParsedMs;
export fn fromNanoseconds(ns_total: i64): ParsedMs;
export fn fromMicroseconds(us_total: i64): ParsedMs;
export fn fromSeconds(sec: i64): ParsedMs;
export fn toMilliseconds(p: ParsedMs): i64;
export fn toMillisecondsF64(p: ParsedMs): f64;
export fn toNanoseconds(p: ParsedMs): i64;
export fn formatParsedMs(p: ParsedMs, out: []u8): usize;
