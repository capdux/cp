// @cap/parse-ms — Zero-heap millisecond breakdown & reconstruction in pure CP
// MIT License — Clean-room implementation inspired by npm `parse-ms` & `to-milliseconds`

import { writeStr, writeU64 } from "cap:fmt";

pub const MS_PER_SECOND: u64 = 1000;
pub const MS_PER_MINUTE: u64 = 60000;
pub const MS_PER_HOUR: u64 = 3600000;
pub const MS_PER_DAY: u64 = 86400000;

pub struct ParsedMs {
    pub days: i64,
    pub hours: i64,
    pub minutes: i64,
    pub seconds: i64,
    pub milliseconds: i64,
    pub microseconds: i64,
    pub nanoseconds: i64,

    pub static fn zero(): ParsedMs {
        return ParsedMs {
            days: 0,
            hours: 0,
            minutes: 0,
            seconds: 0,
            milliseconds: 0,
            microseconds: 0,
            nanoseconds: 0,
        };
    }

    pub fn isZero(): bool {
        return _.days == 0 && _.hours == 0 && _.minutes == 0 && _.seconds == 0 &&
            _.milliseconds == 0 && _.microseconds == 0 && _.nanoseconds == 0;
    }

    pub fn isNegative(): bool {
        return _.days < 0 || _.hours < 0 || _.minutes < 0 || _.seconds < 0 ||
            _.milliseconds < 0 || _.microseconds < 0 || _.nanoseconds < 0;
    }

    pub fn equals(other: ParsedMs): bool {
        return _.days == other.days &&
            _.hours == other.hours &&
            _.minutes == other.minutes &&
            _.seconds == other.seconds &&
            _.milliseconds == other.milliseconds &&
            _.microseconds == other.microseconds &&
            _.nanoseconds == other.nanoseconds;
    }
}

fn signedPart(mag: u64, is_neg: bool): i64 {
    if (mag == 0) {
        return 0;
    }
    return if (is_neg) -(mag as i64) else (mag as i64);
}

fn splitWholeMs(whole_ms: u64, us: u64, ns: u64, is_neg: bool): ParsedMs {
    const d: u64 = whole_ms / MS_PER_DAY;
    const rem_d: u64 = whole_ms % MS_PER_DAY;
    const h: u64 = rem_d / MS_PER_HOUR;
    const rem_h: u64 = rem_d % MS_PER_HOUR;
    const m: u64 = rem_h / MS_PER_MINUTE;
    const rem_m: u64 = rem_h % MS_PER_MINUTE;
    const s: u64 = rem_m / MS_PER_SECOND;
    const ms_rem: u64 = rem_m % MS_PER_SECOND;

    return ParsedMs {
        days: signedPart(d, is_neg),
        hours: signedPart(h, is_neg),
        minutes: signedPart(m, is_neg),
        seconds: signedPart(s, is_neg),
        milliseconds: signedPart(ms_rem, is_neg),
        microseconds: signedPart(us, is_neg),
        nanoseconds: signedPart(ns, is_neg),
    };
}

/// Parse an integer number of milliseconds into days, hours, minutes, seconds, and milliseconds.
/// Uses truncation-toward-zero semantics for negative inputs (e.g. `-1500` -> `-1s, -500ms`).
/// Safely handles `i64` minimum without signed negation overflow.
pub fn parseMs(ms: i64): ParsedMs {
    const is_neg: bool = ms < 0;
    const mag: u64 = if (ms == -9223372036854775807 - 1) {
        9223372036854775808 as u64
    } else if (is_neg) {
        (-ms) as u64
    } else {
        ms as u64
    };
    return splitWholeMs(mag, 0, 0, is_neg);
}

/// Checked floating-point parser: returns `null` for NaN, Infinity, or out-of-range magnitudes.
pub fn parseMsF64Checked(ms: f64): ParsedMs? {
    // NaN check (NaN != NaN)
    if (ms != ms) { return null; }
    const is_neg: bool = ms < 0.0;
    const abs_ms: f64 = if (is_neg) -ms else ms;
    // Reject Infinity and values beyond safe integer precision (2^53 - 1 ms ~ 285,000 years)
    if (abs_ms > 9007199254740991.0) { return null; }
    let whole_ms: u64 = abs_ms as u64;
    const frac: f64 = abs_ms - (whole_ms as f64);
    let total_ns: u64 = (frac * 1000000.0 + 0.5) as u64;
    if (total_ns >= 1000000) {
        whole_ms += 1;
        total_ns -= 1000000;
    }
    const us: u64 = total_ns / 1000;
    const ns: u64 = total_ns % 1000;
    return splitWholeMs(whole_ms, us, ns, is_neg);
}

/// Parse a floating-point number of milliseconds, extracting sub-millisecond microseconds & nanoseconds.
/// Returns `ParsedMs.zero()` if `ms` is NaN or out of safe range.
pub fn parseMsF64(ms: f64): ParsedMs {
    return parseMsF64Checked(ms) ?? ParsedMs.zero();
}

/// Parse an integer number of nanoseconds into full `ParsedMs` (including microseconds & nanoseconds).
pub fn fromNanoseconds(ns_total: i64): ParsedMs {
    const is_neg: bool = ns_total < 0;
    const mag: u64 = if (ns_total == -9223372036854775807 - 1) {
        9223372036854775808 as u64
    } else if (is_neg) {
        (-ns_total) as u64
    } else {
        ns_total as u64
    };
    const whole_ms: u64 = mag / 1000000;
    const sub_ns: u64 = mag % 1000000;
    return splitWholeMs(whole_ms, sub_ns / 1000, sub_ns % 1000, is_neg);
}

/// Parse an integer number of microseconds into `ParsedMs`.
pub fn fromMicroseconds(us_total: i64): ParsedMs {
    const is_neg: bool = us_total < 0;
    const mag: u64 = if (us_total == -9223372036854775807 - 1) {
        9223372036854775808 as u64
    } else if (is_neg) {
        (-us_total) as u64
    } else {
        us_total as u64
    };
    return splitWholeMs(mag / 1000, mag % 1000, 0, is_neg);
}

/// Parse an integer number of seconds into `ParsedMs`.
pub fn fromSeconds(sec: i64): ParsedMs {
    return parseMs(sec * 1000);
}

/// Reconstruct total whole milliseconds from a `ParsedMs` breakdown.
pub fn toMilliseconds(p: ParsedMs): i64 {
    return p.days * 86400000 +
        p.hours * 3600000 +
        p.minutes * 60000 +
        p.seconds * 1000 +
        p.milliseconds;
}

/// Reconstruct total floating-point milliseconds including sub-millisecond microseconds & nanoseconds.
pub fn toMillisecondsF64(p: ParsedMs): f64 {
    const whole: f64 = toMilliseconds(p) as f64;
    const sub: f64 = (p.microseconds as f64) / 1000.0 + (p.nanoseconds as f64) / 1000000.0;
    return whole + sub;
}

/// Reconstruct total nanoseconds from a `ParsedMs` breakdown.
pub fn toNanoseconds(p: ParsedMs): i64 {
    return toMilliseconds(p) * 1000000 + p.microseconds * 1000 + p.nanoseconds;
}

/// Formats `ParsedMs` into a compact human-readable string (e.g. `"15d 11h 23m 20s 1ms"` or `"0ms"`).
pub fn formatParsedMs(p: ParsedMs, out: []u8): usize {
    if (out.len == 0) { return 0; }
    if (p.isZero()) {
        return writeStr("0ms", out, 0);
    }
    let pos: usize = 0;
    if (p.isNegative()) {
        out[pos] = 45; // '-'
        pos += 1;
    }
    let first = true;
    let d: u64 = if (p.days < 0) (-p.days) as u64 else p.days as u64;
    let h: u64 = if (p.hours < 0) (-p.hours) as u64 else p.hours as u64;
    let m: u64 = if (p.minutes < 0) (-p.minutes) as u64 else p.minutes as u64;
    let s: u64 = if (p.seconds < 0) (-p.seconds) as u64 else p.seconds as u64;
    let ms: u64 = if (p.milliseconds < 0) (-p.milliseconds) as u64 else p.milliseconds as u64;
    let us: u64 = if (p.microseconds < 0) (-p.microseconds) as u64 else p.microseconds as u64;
    let ns: u64 = if (p.nanoseconds < 0) (-p.nanoseconds) as u64 else p.nanoseconds as u64;

    if (d > 0) { pos = writeU64(d, out, pos); pos = writeStr("d", out, pos); first = false; }
    if (h > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(h, out, pos); pos = writeStr("h", out, pos); first = false; }
    if (m > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(m, out, pos); pos = writeStr("m", out, pos); first = false; }
    if (s > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(s, out, pos); pos = writeStr("s", out, pos); first = false; }
    if (ms > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(ms, out, pos); pos = writeStr("ms", out, pos); first = false; }
    if (us > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(us, out, pos); pos = writeStr("us", out, pos); first = false; }
    if (ns > 0) { if (!first && pos < out.len) { out[pos] = 32; pos += 1; } pos = writeU64(ns, out, pos); pos = writeStr("ns", out, pos); }
    return pos;
}
