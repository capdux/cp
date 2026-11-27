// @cap/parse-ms — Zero-heap millisecond breakdown & reconstruction in pure CP
// MIT License — Clean-room implementation inspired by npm `parse-ms` & `to-milliseconds`

const MS_PER_SECOND: u64 = 1000;
const MS_PER_MINUTE: u64 = 60000;
const MS_PER_HOUR: u64 = 3600000;
const MS_PER_DAY: u64 = 86400000;

pub struct ParsedMs {
    pub days: i64,
    pub hours: i64,
    pub minutes: i64,
    pub seconds: i64,
    pub milliseconds: i64,
    pub microseconds: i64,
    pub nanoseconds: i64,
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
pub fn parseMs(ms: i64): ParsedMs {
    const is_neg: bool = ms < 0;
    const mag: u64 = if (is_neg) (-ms) as u64 else (ms as u64);
    return splitWholeMs(mag, 0, 0, is_neg);
}

/// Parse a floating-point number of milliseconds, extracting sub-millisecond microseconds & nanoseconds.
pub fn parseMsF64(ms: f64): ParsedMs {
    const is_neg: bool = ms < 0.0;
    const abs_ms: f64 = if (is_neg) -ms else ms;
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

/// Reconstruct total whole milliseconds from a `ParsedMs` breakdown.
pub fn toMilliseconds(p: ParsedMs): i64 {
    return p.days * 86400000 +
        p.hours * 3600000 +
        p.minutes * 60000 +
        p.seconds * 1000 +
        p.milliseconds;
}
