// @cap/ms — Zero-heap millisecond conversion utility
// Clean-room MIT CP implementation of npm Top 6 `ms` (400M+/wk)
// Reuses `@cap/itoa` for fast signed/unsigned integer formatting.

import { formatI64 } from "../../itoa/src/index.cp";

pub const MS_SECOND: i64 = 1000;
pub const MS_MINUTE: i64 = 60000;
pub const MS_HOUR: i64 = 3600000;
pub const MS_DAY: i64 = 86400000;
pub const MS_WEEK: i64 = 604800000;
pub const MS_YEAR: i64 = 31557600000; // 365.25 days

pub struct MsParseResult {
    pub ok: bool,
    pub ms: i64,
}

fn msIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn msIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn msEqIgnoreCase(s: string, start: usize, end: usize, lit: string): bool {
    let n = end - start;
    if (n != lit.len) { return false; }
    let i: usize = 0;
    while (i < n) {
        let c = s[start + i];
        let lo = if (c >= 65 && c <= 90) { c + 32 } else { c };
        if (lo != lit[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn msUnitMultiplier(s: string, start: usize, end: usize): f64 {
    if (start == end) { return 1.0; } // bare number defaults to ms
    if (msEqIgnoreCase(s, start, end, "ms") || msEqIgnoreCase(s, start, end, "msec") || msEqIgnoreCase(s, start, end, "msecs") || msEqIgnoreCase(s, start, end, "millisecond") || msEqIgnoreCase(s, start, end, "milliseconds")) {
        return 1.0;
    }
    if (msEqIgnoreCase(s, start, end, "s") || msEqIgnoreCase(s, start, end, "sec") || msEqIgnoreCase(s, start, end, "secs") || msEqIgnoreCase(s, start, end, "second") || msEqIgnoreCase(s, start, end, "seconds")) {
        return 1000.0;
    }
    if (msEqIgnoreCase(s, start, end, "m") || msEqIgnoreCase(s, start, end, "min") || msEqIgnoreCase(s, start, end, "mins") || msEqIgnoreCase(s, start, end, "minute") || msEqIgnoreCase(s, start, end, "minutes")) {
        return 60000.0;
    }
    if (msEqIgnoreCase(s, start, end, "h") || msEqIgnoreCase(s, start, end, "hr") || msEqIgnoreCase(s, start, end, "hrs") || msEqIgnoreCase(s, start, end, "hour") || msEqIgnoreCase(s, start, end, "hours")) {
        return 3600000.0;
    }
    if (msEqIgnoreCase(s, start, end, "d") || msEqIgnoreCase(s, start, end, "day") || msEqIgnoreCase(s, start, end, "days")) {
        return 86400000.0;
    }
    if (msEqIgnoreCase(s, start, end, "w") || msEqIgnoreCase(s, start, end, "wk") || msEqIgnoreCase(s, start, end, "wks") || msEqIgnoreCase(s, start, end, "week") || msEqIgnoreCase(s, start, end, "weeks")) {
        return 604800000.0;
    }
    if (msEqIgnoreCase(s, start, end, "y") || msEqIgnoreCase(s, start, end, "yr") || msEqIgnoreCase(s, start, end, "yrs") || msEqIgnoreCase(s, start, end, "year") || msEqIgnoreCase(s, start, end, "years")) {
        return 31557600000.0;
    }
    return 0.0;
}

/// Parses a human time string such as `"2 days"`, `"10h"`, `"2.5 hrs"`, `"-3 days"`, `"100"` into milliseconds.
/// Rejects strings > 100 chars (matching npm `ms` DoS guard).
pub fn parseMs(input: string): MsParseResult {
    if (input.len == 0 || input.len > 100) {
        return MsParseResult { ok: false, ms: 0 };
    }
    let i: usize = 0;
    let n = input.len;
    while (i < n && msIsSpace(input[i])) { i = i + 1; }
    if (i >= n) { return MsParseResult { ok: false, ms: 0 }; }

    let neg = false;
    if (input[i] == 45) {
        neg = true;
        i = i + 1;
    } else if (input[i] == 43) {
        i = i + 1;
    }

    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 0.1;
    let has_digits = false;

    while (i < n && msIsDigit(input[i])) {
        int_part = int_part * 10.0 + ((input[i] - 48) as f64);
        has_digits = true;
        i = i + 1;
    }
    if (i < n && input[i] == 46) {
        i = i + 1;
        while (i < n && msIsDigit(input[i])) {
            frac_part = frac_part + ((input[i] - 48) as f64) * frac_scale;
            frac_scale = frac_scale * 0.1;
            has_digits = true;
            i = i + 1;
        }
    }
    if (!has_digits) {
        return MsParseResult { ok: false, ms: 0 };
    }

    while (i < n && msIsSpace(input[i])) { i = i + 1; }
    let u_start = i;
    let u_end = n;
    while (u_end > u_start && msIsSpace(input[u_end - 1])) { u_end = u_end - 1; }

    let mult = msUnitMultiplier(input, u_start, u_end);
    if (mult == 0.0) {
        return MsParseResult { ok: false, ms: 0 };
    }

    let val = (int_part + frac_part) * mult;
    let rounded: i64 = if (neg) { -((val + 0.5) as i64) } else { (val + 0.5) as i64 };
    return MsParseResult { ok: true, ms: rounded };
}

fn msPushStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn msRoundDiv(ms: i64, unit: i64): i64 {
    let neg = ms < 0;
    let abs_ms: u64 = (if (neg) { -ms } else { ms }) as u64;
    let u: u64 = unit as u64;
    let q: i64 = ((abs_ms + (u / 2)) / u) as i64;
    return if (neg) { -q } else { q };
}

fn msFormatUnit(val: i64, short_unit: string, long_singular: string, long_plural: string, long_fmt: bool, out: []u8): usize {
    let tmp: [24]u8 = undefined;
    let n = formatI64(val, tmp[0..24]);
    let p: usize = 0;
    let i: usize = 0;
    while (i < n && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    if (!long_fmt) {
        return msPushStr(out, p, short_unit);
    }
    p = msPushStr(out, p, " ");
    let abs_v = if (val < 0) { -val } else { val };
    if (abs_v == 1) {
        return msPushStr(out, p, long_singular);
    }
    return msPushStr(out, p, long_plural);
}

/// Formats `ms` into compact (`"1m"`, `"2d"`, `"500ms"`) or verbose (`"1 minute"`, `"2 days"`) form.
pub fn formatMs(ms: i64, long_fmt: bool, out: []u8): usize {
    let abs_ms = if (ms < 0) { -ms } else { ms };
    if (abs_ms >= MS_DAY) {
        return msFormatUnit(msRoundDiv(ms, MS_DAY), "d", "day", "days", long_fmt, out);
    }
    if (abs_ms >= MS_HOUR) {
        return msFormatUnit(msRoundDiv(ms, MS_HOUR), "h", "hour", "hours", long_fmt, out);
    }
    if (abs_ms >= MS_MINUTE) {
        return msFormatUnit(msRoundDiv(ms, MS_MINUTE), "m", "minute", "minutes", long_fmt, out);
    }
    if (abs_ms >= MS_SECOND) {
        return msFormatUnit(msRoundDiv(ms, MS_SECOND), "s", "second", "seconds", long_fmt, out);
    }
    return msFormatUnit(ms, "ms", "ms", "ms", long_fmt, out);
}
