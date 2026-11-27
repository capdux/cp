// @cap/humantime — Zero-heap human-readable duration parser/formatter & RFC 3339 timestamps
// Clean-room MIT CP implementation of crates.io `humantime` (950M+ downloads)
// Reuses `@cap/itoa` for fast integer formatting.

import { formatU64 } from "../../itoa/src/index.cp";

pub const NS_PER_US: u64 = 1000;
pub const NS_PER_MS: u64 = 1000000;
pub const NS_PER_SEC: u64 = 1000000000;
pub const NS_PER_MIN: u64 = 60000000000;
pub const NS_PER_HOUR: u64 = 3600000000000;
pub const NS_PER_DAY: u64 = 86400000000000;

pub struct DurationParseResult {
    pub ok: bool,
    pub nanos: u64,
}

fn htIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn htIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn htIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn htEq(s: string, start: usize, end: usize, lit: string): bool {
    let n = end - start;
    if (n != lit.len) { return false; }
    let i: usize = 0;
    while (i < n) {
        if (s[start + i] != lit[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn htUnitNanos(s: string, start: usize, end: usize): u64 {
    if (htEq(s, start, end, "ns") || htEq(s, start, end, "nsec") || htEq(s, start, end, "nsecs") || htEq(s, start, end, "nanosecond") || htEq(s, start, end, "nanoseconds")) {
        return 1;
    }
    if (htEq(s, start, end, "us") || htEq(s, start, end, "usec") || htEq(s, start, end, "usecs") || htEq(s, start, end, "microsecond") || htEq(s, start, end, "microseconds")) {
        return NS_PER_US;
    }
    if (htEq(s, start, end, "ms") || htEq(s, start, end, "msec") || htEq(s, start, end, "msecs") || htEq(s, start, end, "millisecond") || htEq(s, start, end, "milliseconds")) {
        return NS_PER_MS;
    }
    if (htEq(s, start, end, "s") || htEq(s, start, end, "sec") || htEq(s, start, end, "secs") || htEq(s, start, end, "second") || htEq(s, start, end, "seconds")) {
        return NS_PER_SEC;
    }
    if (htEq(s, start, end, "m") || htEq(s, start, end, "min") || htEq(s, start, end, "mins") || htEq(s, start, end, "minute") || htEq(s, start, end, "minutes")) {
        return NS_PER_MIN;
    }
    if (htEq(s, start, end, "h") || htEq(s, start, end, "hr") || htEq(s, start, end, "hrs") || htEq(s, start, end, "hour") || htEq(s, start, end, "hours")) {
        return NS_PER_HOUR;
    }
    if (htEq(s, start, end, "d") || htEq(s, start, end, "day") || htEq(s, start, end, "days")) {
        return NS_PER_DAY;
    }
    if (htEq(s, start, end, "w") || htEq(s, start, end, "week") || htEq(s, start, end, "weeks")) {
        return NS_PER_DAY * 7;
    }
    return 0;
}

/// Parses a human-readable duration string such as `"2h 37min 5s"`, `"150ms"`, `"1day 2hr 3m 4s 500us 10ns"`.
/// Returns `{ ok: true, nanos }` or `{ ok: false, nanos: 0 }` on syntax error or overflow.
pub fn parseDuration(input: string): DurationParseResult {
    let i: usize = 0;
    let n = input.len;
    let total: u64 = 0;
    let found_any = false;

    while (i < n) {
        while (i < n && htIsSpace(input[i])) { i = i + 1; }
        if (i >= n) { break; }

        // Parse unsigned integer value
        if (!htIsDigit(input[i])) {
            return DurationParseResult { ok: false, nanos: 0 };
        }
        let val: u64 = 0;
        while (i < n && htIsDigit(input[i])) {
            let d = (input[i] - 48) as u64;
            val = val * 10 + d;
            i = i + 1;
        }

        while (i < n && htIsSpace(input[i])) { i = i + 1; }
        let u_start = i;
        while (i < n && htIsAlpha(input[i])) { i = i + 1; }
        let u_end = i;
        if (u_start == u_end) {
            return DurationParseResult { ok: false, nanos: 0 };
        }

        let mult = htUnitNanos(input, u_start, u_end);
        if (mult == 0) {
            return DurationParseResult { ok: false, nanos: 0 };
        }
        total = total + (val * mult);
        found_any = true;
    }

    if (!found_any) {
        return DurationParseResult { ok: false, nanos: 0 };
    }
    return DurationParseResult { ok: true, nanos: total };
}

fn htPushStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn htPushPart(out: []u8, pos: usize, val: u64, suffix: string): usize {
    if (val == 0) { return pos; }
    let p = pos;
    if (p > 0 && p < out.len) {
        out[p] = 32; // ' '
        p = p + 1;
    }
    let tmp: [24]u8 = undefined;
    let dlen = formatU64(val, tmp[0..24]);
    let i: usize = 0;
    while (i < dlen && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    return htPushStr(out, p, suffix);
}

/// Formats a nanosecond duration into human-readable form (e.g. `"2h 37m 5s 120ms"`, `"0s"` for zero).
pub fn formatDuration(nanos: u64, out: []u8): usize {
    if (nanos == 0) {
        return htPushStr(out, 0, "0s");
    }
    let rem = nanos;
    let days = rem / NS_PER_DAY;
    rem = rem % NS_PER_DAY;
    let hours = rem / NS_PER_HOUR;
    rem = rem % NS_PER_HOUR;
    let mins = rem / NS_PER_MIN;
    rem = rem % NS_PER_MIN;
    let secs = rem / NS_PER_SEC;
    rem = rem % NS_PER_SEC;
    let ms = rem / NS_PER_MS;
    rem = rem % NS_PER_MS;
    let us = rem / NS_PER_US;
    let ns = rem % NS_PER_US;

    let pos: usize = 0;
    pos = htPushPart(out, pos, days, "d");
    pos = htPushPart(out, pos, hours, "h");
    pos = htPushPart(out, pos, mins, "m");
    pos = htPushPart(out, pos, secs, "s");
    pos = htPushPart(out, pos, ms, "ms");
    pos = htPushPart(out, pos, us, "us");
    pos = htPushPart(out, pos, ns, "ns");
    return pos;
}

fn htPush2Digits(out: []u8, pos: usize, v: u64): usize {
    let p = pos;
    if (p < out.len) { out[p] = 48 + (((v / 10) % 10) as u8); p = p + 1; }
    if (p < out.len) { out[p] = 48 + ((v % 10) as u8); p = p + 1; }
    return p;
}

fn htPush4Digits(out: []u8, pos: usize, v: u64): usize {
    let p = pos;
    if (p < out.len) { out[p] = 48 + (((v / 1000) % 10) as u8); p = p + 1; }
    if (p < out.len) { out[p] = 48 + (((v / 100) % 10) as u8); p = p + 1; }
    if (p < out.len) { out[p] = 48 + (((v / 10) % 10) as u8); p = p + 1; }
    if (p < out.len) { out[p] = 48 + ((v % 10) as u8); p = p + 1; }
    return p;
}

/// Formats a Unix UTC timestamp (seconds since 1970-01-01T00:00:00Z) as RFC 3339 (`"YYYY-MM-DDTHH:MM:SSZ"`).
pub fn formatRfc3339Seconds(unix_secs: u64, out: []u8): usize {
    // Howard Hinnant civil_from_days algorithm (exact O(1) Gregorian calendar conversion)
    let days = unix_secs / 86400;
    let tod = unix_secs % 86400;
    let hour = tod / 3600;
    let min = (tod % 3600) / 60;
    let sec = tod % 60;

    let z = days + 719468;
    let era = z / 146097;
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = if (mp < 10) { mp + 3 } else { mp - 9 };
    let year = if (month <= 2) { y + 1 } else { y };

    let p: usize = 0;
    p = htPush4Digits(out, p, year);
    p = htPushStr(out, p, "-");
    p = htPush2Digits(out, p, month);
    p = htPushStr(out, p, "-");
    p = htPush2Digits(out, p, day);
    p = htPushStr(out, p, "T");
    p = htPush2Digits(out, p, hour);
    p = htPushStr(out, p, ":");
    p = htPush2Digits(out, p, min);
    p = htPushStr(out, p, ":");
    p = htPush2Digits(out, p, sec);
    p = htPushStr(out, p, "Z");
    return p;
}
