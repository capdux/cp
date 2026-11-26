// Type definitions for CP

export class ParsedMs {
    days: i64,
    hours: i64,
    minutes: i64,
    seconds: i64,
    milliseconds: i64,
    microseconds: i64,
    nanoseconds: i64,
}

export fn signedPart(mag: u64, is_neg: bool): i64;
export fn splitWholeMs(whole_ms: u64, us: u64, ns: u64, is_neg: bool): ParsedMs;
export fn parseMs(ms: i64): ParsedMs;
export fn parseMsF64(ms: f64): ParsedMs;
export fn toMilliseconds(p: ParsedMs): i64;
