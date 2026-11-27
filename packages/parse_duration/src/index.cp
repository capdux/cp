// @cap/parse-duration — Zero-heap human-readable duration parser & formatter.
// Clean-room MIT implementation modeled on crates.io `parse_duration` / `duration-str`.

import { isSpace, isDigit, isAlpha, toLowerByte, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

const PDUR_U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
const PDUR_NANOS_PER_SEC: u64 = 1000000000;
const PDUR_SECS_PER_MIN: u64 = 60;
const PDUR_SECS_PER_HOUR: u64 = 3600;
const PDUR_SECS_PER_DAY: u64 = 86400;
const PDUR_SECS_PER_WEEK: u64 = 604800;
const PDUR_SECS_PER_YEAR: u64 = 31536000;

fn pdurCheckedAddU64(a: u64, b: u64): u64? {
    if (a > PDUR_U64_MAX - b) { return null; }
    return a + b;
}

fn pdurCheckedMulU64(a: u64, b: u64): u64? {
    if (a == 0 || b == 0) { return 0; }
    if (a > PDUR_U64_MAX / b) { return null; }
    return a * b;
}

// Returns unit kind:
// 0 = invalid, 1 = ns, 2 = us, 3 = ms, 4 = s, 5 = m, 6 = h, 7 = d, 8 = w, 9 = y
fn pdurMatchUnit(u: string): u32 {
    if (strEq(u, "ns") || strEq(u, "nsec") || strEq(u, "nanosecond") || strEq(u, "nanoseconds")) {
        return 1;
    }
    if (strEq(u, "us") || strEq(u, "usec") || strEq(u, "microsecond") || strEq(u, "microseconds")) {
        return 2;
    }
    if (strEq(u, "ms") || strEq(u, "msec") || strEq(u, "millisecond") || strEq(u, "milliseconds")) {
        return 3;
    }
    if (strEq(u, "s") || strEq(u, "sec") || strEq(u, "second") || strEq(u, "seconds")) {
        return 4;
    }
    if (strEq(u, "m") || strEq(u, "min") || strEq(u, "minute") || strEq(u, "minutes")) {
        return 5;
    }
    if (strEq(u, "h") || strEq(u, "hr") || strEq(u, "hour") || strEq(u, "hours")) {
        return 6;
    }
    if (strEq(u, "d") || strEq(u, "day") || strEq(u, "days")) {
        return 7;
    }
    if (strEq(u, "w") || strEq(u, "wk") || strEq(u, "week") || strEq(u, "weeks")) {
        return 8;
    }
    if (strEq(u, "y") || strEq(u, "yr") || strEq(u, "year") || strEq(u, "years")) {
        return 9;
    }
    return 0;
}

fn pdurAppendPart(out: []u8, pos: usize, val: u64, suffix: string): usize {
    if (val == 0) { return pos; }
    let p: usize = pos;
    if (p > 0 && p < out.len) {
        out[p] = 32;
        p = p + 1;
    }
    p = writeU64(val, out, p);
    p = writeStr(suffix, out, p);
    return p;
}

/// Parsed duration represented as whole seconds and sub-second nanoseconds (`0..999_999_999`).
pub struct ParsedDuration {
    pub secs: u64,
    pub nanos: u32,

    pub fn isZero(): bool {
        return _.secs == 0 && _.nanos == 0;
    }

    pub fn totalSecs(): u64 {
        return _.secs;
    }

    pub fn totalMillis(): u64 {
        let ms_secs = pdurCheckedMulU64(_.secs, 1000);
        if (ms_secs == null) { return PDUR_U64_MAX; }
        let extra: u64 = (_.nanos / 1000000) as u64;
        let sum = pdurCheckedAddU64(ms_secs ?? 0, extra);
        return sum ?? PDUR_U64_MAX;
    }

    pub fn totalNanos(): u64? {
        let ns_secs = pdurCheckedMulU64(_.secs, PDUR_NANOS_PER_SEC);
        if (ns_secs == null) { return null; }
        return pdurCheckedAddU64(ns_secs ?? 0, _.nanos as u64);
    }
}

/// Parses a human-readable duration string (e.g. `"2h 30m 15s"`, `"1.5h"`, `"250µs"`, `"1h + 30m"`)
/// into a `ParsedDuration`, or returns `null` on empty, malformed, negative, unitless, or overflowing input.
pub fn parseDuration(input: string): ParsedDuration? {
    let len: usize = input.len;
    let i: usize = 0;
    while (i < len && isSpace(input[i])) {
        i = i + 1;
    }
    if (i >= len) { return null; }

    let total_secs: u64 = 0;
    let total_nanos: u64 = 0;
    let parts: usize = 0;

    while (i < len) {
        if (parts > 0) {
            let had_sep: bool = false;
            while (i < len && isSpace(input[i])) {
                i = i + 1;
            }
            if (i >= len) { break; }
            if (input[i] == 44 || input[i] == 43) {
                had_sep = true;
                i = i + 1;
                while (i < len && isSpace(input[i])) {
                    i = i + 1;
                }
            }
            if (i >= len) {
                if (had_sep) { return null; }
                break;
            }
        }

        // Parse unsigned number (integer + optional fractional part)
        let int_part: u64 = 0;
        let frac_val: u64 = 0;
        let frac_scale: u64 = 1;
        let has_digits: bool = false;

        while (i < len && isDigit(input[i])) {
            has_digits = true;
            let digit: u64 = (input[i] - 48) as u64;
            let m = pdurCheckedMulU64(int_part, 10);
            if (m == null) { return null; }
            let a = pdurCheckedAddU64(m ?? 0, digit);
            if (a == null) { return null; }
            int_part = a ?? 0;
            i = i + 1;
        }

        if (i < len && input[i] == 46) {
            i = i + 1;
            let has_frac_digits: bool = false;
            while (i < len && isDigit(input[i])) {
                has_frac_digits = true;
                has_digits = true;
                if (frac_scale < PDUR_NANOS_PER_SEC) {
                    frac_val = frac_val * 10 + ((input[i] - 48) as u64);
                    frac_scale = frac_scale * 10;
                }
                i = i + 1;
            }
            if (!has_frac_digits) { return null; }
        }

        if (!has_digits) { return null; }

        // Optional whitespace between number and unit
        while (i < len && isSpace(input[i])) {
            i = i + 1;
        }
        if (i >= len) { return null; }

        // Parse unit (supports UTF-8 'µs' [0xC2 0xB5 's'] and 'μs' [0xCE 0xBC 's'])
        let unit_kind: u32 = 0;
        if (i + 2 < len && ((input[i] == 0xC2 && input[i + 1] == 0xB5) || (input[i] == 0xCE && input[i + 1] == 0xBC)) && toLowerByte(input[i + 2]) == 115) {
            unit_kind = 2;
            i = i + 3;
            if (i < len && isAlpha(input[i])) { return null; }
        } else {
            let ubuf: [16]u8 = undefined;
            let ulen: usize = 0;
            while (i < len && isAlpha(input[i])) {
                if (ulen >= 16) { return null; }
                ubuf[ulen] = toLowerByte(input[i]);
                ulen = ulen + 1;
                i = i + 1;
            }
            if (ulen == 0) { return null; }
            unit_kind = pdurMatchUnit(ubuf[0..ulen]);
        }

        if (unit_kind == 0) { return null; }

        let add_secs: u64 = 0;
        let add_nanos: u64 = 0;

        if (unit_kind <= 3) {
            let ns_per_unit: u64 = if (unit_kind == 1) { 1 } else if (unit_kind == 2) { 1000 } else { 1000000 };
            let units_per_sec: u64 = PDUR_NANOS_PER_SEC / ns_per_unit;
            add_secs = int_part / units_per_sec;
            add_nanos = (int_part % units_per_sec) * ns_per_unit;
            if (frac_val > 0) {
                add_nanos = add_nanos + ((frac_val * ns_per_unit) / frac_scale);
            }
        } else {
            let unit_s: u64 = 1;
            if (unit_kind == 5) { unit_s = PDUR_SECS_PER_MIN; }
            else if (unit_kind == 6) { unit_s = PDUR_SECS_PER_HOUR; }
            else if (unit_kind == 7) { unit_s = PDUR_SECS_PER_DAY; }
            else if (unit_kind == 8) { unit_s = PDUR_SECS_PER_WEEK; }
            else if (unit_kind == 9) { unit_s = PDUR_SECS_PER_YEAR; }

            let s_int = pdurCheckedMulU64(int_part, unit_s);
            if (s_int == null) { return null; }
            add_secs = s_int ?? 0;

            if (frac_val > 0) {
                let prod: u64 = frac_val * unit_s;
                let extra_secs: u64 = prod / frac_scale;
                let rem: u64 = prod % frac_scale;
                let s_sum = pdurCheckedAddU64(add_secs, extra_secs);
                if (s_sum == null) { return null; }
                add_secs = s_sum ?? 0;
                add_nanos = rem * (PDUR_NANOS_PER_SEC / frac_scale);
            }
        }

        let carry: u64 = add_nanos / PDUR_NANOS_PER_SEC;
        add_nanos = add_nanos % PDUR_NANOS_PER_SEC;

        let s1 = pdurCheckedAddU64(add_secs, carry);
        if (s1 == null) { return null; }
        let s2 = pdurCheckedAddU64(total_secs, s1 ?? 0);
        if (s2 == null) { return null; }
        total_secs = s2 ?? 0;

        total_nanos = total_nanos + add_nanos;
        if (total_nanos >= PDUR_NANOS_PER_SEC) {
            let s3 = pdurCheckedAddU64(total_secs, 1);
            if (s3 == null) { return null; }
            total_secs = s3 ?? 0;
            total_nanos = total_nanos - PDUR_NANOS_PER_SEC;
        }

        parts = parts + 1;
    }

    if (parts == 0) { return null; }
    return ParsedDuration {
        secs: total_secs,
        nanos: total_nanos as u32,
    };
}

/// Formats `d` into human-readable form (e.g. `"1h 30m 5s"`, `"250ms"`, `"0s"`) into `out`.
/// Returns the number of bytes written.
pub fn formatDuration(d: ParsedDuration, out: []u8): usize {
    if (d.isZero()) {
        return writeStr("0s", out, 0);
    }
    let rem_s: u64 = d.secs;
    let years: u64 = rem_s / PDUR_SECS_PER_YEAR;
    rem_s = rem_s % PDUR_SECS_PER_YEAR;
    let days: u64 = rem_s / PDUR_SECS_PER_DAY;
    rem_s = rem_s % PDUR_SECS_PER_DAY;
    let hours: u64 = rem_s / PDUR_SECS_PER_HOUR;
    rem_s = rem_s % PDUR_SECS_PER_HOUR;
    let mins: u64 = rem_s / PDUR_SECS_PER_MIN;
    let secs: u64 = rem_s % PDUR_SECS_PER_MIN;

    let rem_ns: u64 = d.nanos as u64;
    let ms: u64 = rem_ns / 1000000;
    rem_ns = rem_ns % 1000000;
    let us: u64 = rem_ns / 1000;
    let ns: u64 = rem_ns % 1000;

    let pos: usize = 0;
    pos = pdurAppendPart(out, pos, years, "y");
    pos = pdurAppendPart(out, pos, days, "d");
    pos = pdurAppendPart(out, pos, hours, "h");
    pos = pdurAppendPart(out, pos, mins, "m");
    pos = pdurAppendPart(out, pos, secs, "s");
    pos = pdurAppendPart(out, pos, ms, "ms");
    pos = pdurAppendPart(out, pos, us, "us");
    pos = pdurAppendPart(out, pos, ns, "ns");
    return pos;
}
