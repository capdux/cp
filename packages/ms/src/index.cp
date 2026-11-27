// @cap/ms — Zero-heap millisecond conversion utility
// Clean-room MIT CP implementation of npm Top 6 `ms` (400M+/wk)
// Reuses `@cap/itoa` for fast signed/unsigned integer formatting.

import { formatI64 } from "../../itoa/src/index.cp";
import { isSpace, isDigit, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

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

fn msUnitMultiplier(s: string, start: usize, end: usize): f64 {
    if (start == end) { return 1.0; } // bare number defaults to ms
    const u = s[start..end];
    if (eqIgnoreCase(u, "ms") || eqIgnoreCase(u, "msec") || eqIgnoreCase(u, "msecs") || eqIgnoreCase(u, "millisecond") || eqIgnoreCase(u, "milliseconds")) {
        return 1.0;
    }
    if (eqIgnoreCase(u, "s") || eqIgnoreCase(u, "sec") || eqIgnoreCase(u, "secs") || eqIgnoreCase(u, "second") || eqIgnoreCase(u, "seconds")) {
        return 1000.0;
    }
    if (eqIgnoreCase(u, "m") || eqIgnoreCase(u, "min") || eqIgnoreCase(u, "mins") || eqIgnoreCase(u, "minute") || eqIgnoreCase(u, "minutes")) {
        return 60000.0;
    }
    if (eqIgnoreCase(u, "h") || eqIgnoreCase(u, "hr") || eqIgnoreCase(u, "hrs") || eqIgnoreCase(u, "hour") || eqIgnoreCase(u, "hours")) {
        return 3600000.0;
    }
    if (eqIgnoreCase(u, "d") || eqIgnoreCase(u, "day") || eqIgnoreCase(u, "days")) {
        return 86400000.0;
    }
    if (eqIgnoreCase(u, "w") || eqIgnoreCase(u, "wk") || eqIgnoreCase(u, "wks") || eqIgnoreCase(u, "week") || eqIgnoreCase(u, "weeks")) {
        return 604800000.0;
    }
    if (eqIgnoreCase(u, "y") || eqIgnoreCase(u, "yr") || eqIgnoreCase(u, "yrs") || eqIgnoreCase(u, "year") || eqIgnoreCase(u, "years")) {
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
    while (i < n && isSpace(input[i])) { i = i + 1; }
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

    while (i < n && isDigit(input[i])) {
        int_part = int_part * 10.0 + ((input[i] - 48) as f64);
        has_digits = true;
        i = i + 1;
    }
    if (i < n && input[i] == 46) {
        i = i + 1;
        while (i < n && isDigit(input[i])) {
            frac_part = frac_part + ((input[i] - 48) as f64) * frac_scale;
            frac_scale = frac_scale * 0.1;
            has_digits = true;
            i = i + 1;
        }
    }
    if (!has_digits) {
        return MsParseResult { ok: false, ms: 0 };
    }

    while (i < n && isSpace(input[i])) { i = i + 1; }
    let u_start = i;
    let u_end = n;
    while (u_end > u_start && isSpace(input[u_end - 1])) { u_end = u_end - 1; }

    let mult = msUnitMultiplier(input, u_start, u_end);
    if (mult == 0.0) {
        return MsParseResult { ok: false, ms: 0 };
    }

    let val = (int_part + frac_part) * mult;
    let rounded: i64 = if (neg) { -((val + 0.5) as i64) } else { (val + 0.5) as i64 };
    return MsParseResult { ok: true, ms: rounded };
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
    let p: usize = writeStr(tmp[0..n], out, 0);
    if (!long_fmt) {
        return writeStr(short_unit, out, p);
    }
    p = writeStr(" ", out, p);
    let abs_v = if (val < 0) { -val } else { val };
    if (abs_v == 1) {
        return writeStr(long_singular, out, p);
    }
    return writeStr(long_plural, out, p);
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
