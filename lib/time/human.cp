// @cap/ms — Pure CP, zero-dependency, zero-allocation duration parser & formatter
// Clean-room MIT implementation inspired by npm `ms` and Rust `humantime`.
// Features:
//   - `parseMs`: single-unit signed duration parsing with decimals & all case-insensitive aliases
//   - `parseDurationNs`: multi-segment compound unsigned nanosecond duration parsing with overflow protection
//   - `formatShort` / `formatLong`: npm `ms`-compatible rounded short & pluralized long formatting
//   - `formatDurationNs`: Rust `humantime`-compatible exact compound formatting (round-trip with `parseDurationNs`)
//   - Zero heap allocation, zero `unsafe`, zero floating-point drift

pub err MsError {
    EmptyInput,
    InvalidFormat,
    UnknownUnit,
    Overflow,
    BufferTooSmall,
}

// Nanosecond time constants
pub const NS_PER_US: u64 = 1000;
pub const NS_PER_MS: u64 = 1000000;
pub const NS_PER_SEC: u64 = 1000000000;
pub const NS_PER_MIN: u64 = 60000000000;
pub const NS_PER_HOUR: u64 = 3600000000000;
pub const NS_PER_DAY: u64 = 86400000000000;
pub const NS_PER_WEEK: u64 = 604800000000000;

// Millisecond time constants (MS_PER_YEAR = 365.25 days, matching npm `ms`)
pub const MS_PER_SEC: i64 = 1000;
pub const MS_PER_MIN: i64 = 60000;
pub const MS_PER_HOUR: i64 = 3600000;
pub const MS_PER_DAY: i64 = 86400000;
pub const MS_PER_WEEK: i64 = 604800000;
pub const MS_PER_YEAR: i64 = 31557600000;

const NS_PER_YEAR: u64 = 31557600000000000;
const MAX_I64_U64: u64 = 0x7FFFFFFFFFFFFFFF;
const MIN_I64_MAG: u64 = MAX_I64_U64 + 1;
const MAX_U64: u64 = MAX_I64_U64 * 2 + 1;
const MIN_I64: i64 = -9223372036854775807 - 1;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: []const u8, lower_lit: []const u8): bool {
    if (a.len != lower_lit.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != lower_lit[i]) {
            return false;
        }
    }
    return true;
}

fn checkedAddU64(a: u64, b: u64): u64 ! MsError {
    if (a > MAX_U64 - b) {
        return MsError.Overflow;
    }
    return a + b;
}

fn checkedMulU64(a: u64, b: u64): u64 ! MsError {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > MAX_U64 / b) {
        return MsError.Overflow;
    }
    return a * b;
}

fn absI64ToU64(v: i64): u64 {
    if (v >= 0) {
        return u64(v);
    }
    if (v == MIN_I64) {
        return MIN_I64_MAG;
    }
    return u64(-v);
}

fn roundDivU64(num: u64, denom: u64): u64 {
    const q = num / denom;
    const r = num % denom;
    if (r >= (denom - r)) {
        return q + 1;
    }
    return q;
}

fn lookupMsUnit(unit: []const u8): u64 ! MsError {
    if (unit.len == 0) {
        return 1;
    }
    if (eqIgnoreCase(unit, "ms") ||
        eqIgnoreCase(unit, "msec") ||
        eqIgnoreCase(unit, "msecs") ||
        eqIgnoreCase(unit, "millisecond") ||
        eqIgnoreCase(unit, "milliseconds")) {
        return 1;
    }
    if (eqIgnoreCase(unit, "s") ||
        eqIgnoreCase(unit, "sec") ||
        eqIgnoreCase(unit, "secs") ||
        eqIgnoreCase(unit, "second") ||
        eqIgnoreCase(unit, "seconds")) {
        return u64(MS_PER_SEC);
    }
    if (eqIgnoreCase(unit, "m") ||
        eqIgnoreCase(unit, "min") ||
        eqIgnoreCase(unit, "mins") ||
        eqIgnoreCase(unit, "minute") ||
        eqIgnoreCase(unit, "minutes")) {
        return u64(MS_PER_MIN);
    }
    if (eqIgnoreCase(unit, "h") ||
        eqIgnoreCase(unit, "hr") ||
        eqIgnoreCase(unit, "hrs") ||
        eqIgnoreCase(unit, "hour") ||
        eqIgnoreCase(unit, "hours")) {
        return u64(MS_PER_HOUR);
    }
    if (eqIgnoreCase(unit, "d") ||
        eqIgnoreCase(unit, "day") ||
        eqIgnoreCase(unit, "days")) {
        return u64(MS_PER_DAY);
    }
    if (eqIgnoreCase(unit, "w") ||
        eqIgnoreCase(unit, "week") ||
        eqIgnoreCase(unit, "weeks")) {
        return u64(MS_PER_WEEK);
    }
    if (eqIgnoreCase(unit, "y") ||
        eqIgnoreCase(unit, "yr") ||
        eqIgnoreCase(unit, "yrs") ||
        eqIgnoreCase(unit, "year") ||
        eqIgnoreCase(unit, "years")) {
        return u64(MS_PER_YEAR);
    }
    return MsError.UnknownUnit;
}

fn lookupNsUnit(unit: []const u8): u64 ! MsError {
    if (unit.len == 0) {
        return MsError.InvalidFormat;
    }
    // Support UTF-8 micro sign U+00B5 (0xC2 0xB5) followed by 's' or 'S'
    if (unit.len == 3 && unit[0] == 0xC2 && unit[1] == 0xB5 && toLowerAscii(unit[2]) == 's') {
        return NS_PER_US;
    }
    if (eqIgnoreCase(unit, "ns") ||
        eqIgnoreCase(unit, "nsec") ||
        eqIgnoreCase(unit, "nsecs") ||
        eqIgnoreCase(unit, "nanosecond") ||
        eqIgnoreCase(unit, "nanoseconds")) {
        return 1;
    }
    if (eqIgnoreCase(unit, "us") ||
        eqIgnoreCase(unit, "usec") ||
        eqIgnoreCase(unit, "usecs") ||
        eqIgnoreCase(unit, "microsecond") ||
        eqIgnoreCase(unit, "microseconds")) {
        return NS_PER_US;
    }
    if (eqIgnoreCase(unit, "ms") ||
        eqIgnoreCase(unit, "msec") ||
        eqIgnoreCase(unit, "msecs") ||
        eqIgnoreCase(unit, "millisecond") ||
        eqIgnoreCase(unit, "milliseconds")) {
        return NS_PER_MS;
    }
    if (eqIgnoreCase(unit, "s") ||
        eqIgnoreCase(unit, "sec") ||
        eqIgnoreCase(unit, "secs") ||
        eqIgnoreCase(unit, "second") ||
        eqIgnoreCase(unit, "seconds")) {
        return NS_PER_SEC;
    }
    if (eqIgnoreCase(unit, "m") ||
        eqIgnoreCase(unit, "min") ||
        eqIgnoreCase(unit, "mins") ||
        eqIgnoreCase(unit, "minute") ||
        eqIgnoreCase(unit, "minutes")) {
        return NS_PER_MIN;
    }
    if (eqIgnoreCase(unit, "h") ||
        eqIgnoreCase(unit, "hr") ||
        eqIgnoreCase(unit, "hrs") ||
        eqIgnoreCase(unit, "hour") ||
        eqIgnoreCase(unit, "hours")) {
        return NS_PER_HOUR;
    }
    if (eqIgnoreCase(unit, "d") ||
        eqIgnoreCase(unit, "day") ||
        eqIgnoreCase(unit, "days")) {
        return NS_PER_DAY;
    }
    if (eqIgnoreCase(unit, "w") ||
        eqIgnoreCase(unit, "week") ||
        eqIgnoreCase(unit, "weeks")) {
        return NS_PER_WEEK;
    }
    if (eqIgnoreCase(unit, "y") ||
        eqIgnoreCase(unit, "yr") ||
        eqIgnoreCase(unit, "yrs") ||
        eqIgnoreCase(unit, "year") ||
        eqIgnoreCase(unit, "years")) {
        return NS_PER_YEAR;
    }
    return MsError.UnknownUnit;
}

fn writeBytes(out: []u8, pos: *usize, src: []const u8): void ! MsError {
    if (src.len > out.len - pos.*) {
        return MsError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
}

fn writeByte(out: []u8, pos: *usize, b: u8): void ! MsError {
    if (pos.* >= out.len) {
        return MsError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn writeU64(out: []u8, pos: *usize, value: u64): void ! MsError {
    let tmp: [20]u8 = [0; 20];
    let count: usize = 0;
    let v = value;
    if (v == 0) {
        tmp[0] = '0';
        count = 1;
    } else {
        while (v > 0) {
            tmp[count] = (48 + (v % 10)) as u8;
            v /= 10;
            count += 1;
        }
    }
    if (count > out.len - pos.*) {
        return MsError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        out[pos.* + i] = tmp[count - 1 - i];
    }
    pos.* += count;
}

/// Parses an npm `ms`-compatible duration string into signed milliseconds.
/// Supports bare numbers (`"100"`), optional sign (`"-3 days"`, `"+500ms"`),
/// decimals (`"2.5 hrs"`, `".5s"`), and all case-insensitive unit aliases.
pub fn parseMs(input: []const u8): i64 ! MsError {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isSpace(input[start])) {
        start += 1;
    }
    while (end > start && isSpace(input[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return MsError.EmptyInput;
    }

    let i: usize = start;
    let is_neg = false;
    if (input[i] == '-') {
        is_neg = true;
        i += 1;
    } else if (input[i] == '+') {
        i += 1;
    }

    if (i >= end) {
        return MsError.InvalidFormat;
    }

    let int_part: u64 = 0;
    let has_int_digits = false;
    while (i < end && isDigit(input[i])) {
        has_int_digits = true;
        const digit = u64(input[i] - '0');
        int_part = checkedMulU64(int_part, 10)?;
        int_part = checkedAddU64(int_part, digit)?;
        i += 1;
    }

    let frac_val: u128 = 0;
    let frac_scale: u128 = 1;
    let frac_kept: usize = 0;
    let has_frac_digits = false;
    if (i < end && input[i] == '.') {
        i += 1;
        while (i < end && isDigit(input[i])) {
            has_frac_digits = true;
            if (frac_kept < 18) {
                frac_val = frac_val * 10 + u128(input[i] - '0');
                frac_scale = frac_scale * 10;
                frac_kept += 1;
            }
            i += 1;
        }
    }

    if (!has_int_digits && !has_frac_digits) {
        return MsError.InvalidFormat;
    }

    // Skip optional spaces between number and unit
    while (i < end && isSpace(input[i])) {
        i += 1;
    }

    let unit_ms: u64 = 1;
    if (i < end) {
        const unit_start = i;
        while (i < end) {
            if (isSpace(input[i])) {
                return MsError.InvalidFormat;
            }
            if (!isAlpha(input[i])) {
                return MsError.InvalidFormat;
            }
            i += 1;
        }
        unit_ms = lookupMsUnit(input[unit_start..end])?;
    }

    const int_ms = checkedMulU64(int_part, unit_ms)?;
    let frac_ms: u64 = 0;
    if (frac_val > 0) {
        const scaled: u128 = frac_val * u128(unit_ms);
        const rounded: u128 = (scaled + (frac_scale / 2)) / frac_scale;
        if (rounded > u128(MAX_U64)) {
            return MsError.Overflow;
        }
        frac_ms = rounded as u64;
    }

    const total_mag = checkedAddU64(int_ms, frac_ms)?;
    if (!is_neg) {
        if (total_mag > MAX_I64_U64) {
            return MsError.Overflow;
        }
        return total_mag as i64;
    }

    if (total_mag == 0) {
        return 0;
    }
    if (total_mag > MIN_I64_MAG) {
        return MsError.Overflow;
    }
    if (total_mag == MIN_I64_MAG) {
        return MIN_I64;
    }
    return -(total_mag as i64);
}

/// Parses a Rust `humantime`-compatible compound unsigned duration into nanoseconds.
/// Supports multi-segment inputs such as `"1h 30m 15s 500ms"`, `"2d 4h"`, `"100us"`, `"500ns"`.
pub fn parseDurationNs(input: []const u8): u64 ! MsError {
    let start: usize = 0;
    let end: usize = input.len;
    while (start < end && isSpace(input[start])) {
        start += 1;
    }
    while (end > start && isSpace(input[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return MsError.EmptyInput;
    }

    let i: usize = start;
    let total_ns: u64 = 0;

    while (i < end) {
        if (!isDigit(input[i])) {
            return MsError.InvalidFormat;
        }

        let seg_val: u64 = 0;
        while (i < end && isDigit(input[i])) {
            const digit = u64(input[i] - '0');
            seg_val = checkedMulU64(seg_val, 10)?;
            seg_val = checkedAddU64(seg_val, digit)?;
            i += 1;
        }

        // Skip optional whitespace between number and unit
        while (i < end && isSpace(input[i])) {
            i += 1;
        }

        if (i >= end) {
            return MsError.InvalidFormat;
        }

        const unit_start = i;
        while (i < end) {
            if (isAlpha(input[i])) {
                i += 1;
            } else if (i + 1 < end && input[i] == 0xC2 && input[i + 1] == 0xB5) {
                i += 2;
            } else {
                break;
            }
        }

        if (i == unit_start) {
            return MsError.InvalidFormat;
        }

        const unit_ns = lookupNsUnit(input[unit_start..i])?;
        const seg_ns = checkedMulU64(seg_val, unit_ns)?;
        total_ns = checkedAddU64(total_ns, seg_ns)?;

        // Skip whitespace between segments; compact form like "1h30m" is also accepted
        while (i < end && isSpace(input[i])) {
            i += 1;
        }
    }

    return total_ns;
}

/// Formats signed milliseconds into npm `ms` short format (e.g. `"2d"`, `"10h"`, `"-3m"`, `"500ms"`).
pub fn formatShort(ms: i64, out: []u8): ([]const u8) ! MsError {
    const is_neg = ms < 0;
    const abs_ms = absI64ToU64(ms);

    let value: u64 = abs_ms;
    let suffix: []const u8 = "ms";

    if (abs_ms >= u64(MS_PER_DAY)) {
        value = roundDivU64(abs_ms, u64(MS_PER_DAY));
        suffix = "d";
    } else if (abs_ms >= u64(MS_PER_HOUR)) {
        value = roundDivU64(abs_ms, u64(MS_PER_HOUR));
        suffix = "h";
    } else if (abs_ms >= u64(MS_PER_MIN)) {
        value = roundDivU64(abs_ms, u64(MS_PER_MIN));
        suffix = "m";
    } else if (abs_ms >= u64(MS_PER_SEC)) {
        value = roundDivU64(abs_ms, u64(MS_PER_SEC));
        suffix = "s";
    }

    let pos: usize = 0;
    if (is_neg && value > 0) {
        writeByte(out, *pos, '-')?;
    }
    writeU64(out, *pos, value)?;
    writeBytes(out, *pos, suffix)?;
    return out[0..pos];
}

/// Formats signed milliseconds into npm `ms` long format with automatic pluralization
/// (e.g. `"1 day"`, `"2 days"`, `"1 hour"`, `"-3 minutes"`, `"500 ms"`).
pub fn formatLong(ms: i64, out: []u8): ([]const u8) ! MsError {
    const is_neg = ms < 0;
    const abs_ms = absI64ToU64(ms);

    let value: u64 = abs_ms;
    let suffix: []const u8 = " ms";

    if (abs_ms >= u64(MS_PER_DAY)) {
        value = roundDivU64(abs_ms, u64(MS_PER_DAY));
        if (value == 1) {
            suffix = " day";
        } else {
            suffix = " days";
        }
    } else if (abs_ms >= u64(MS_PER_HOUR)) {
        value = roundDivU64(abs_ms, u64(MS_PER_HOUR));
        if (value == 1) {
            suffix = " hour";
        } else {
            suffix = " hours";
        }
    } else if (abs_ms >= u64(MS_PER_MIN)) {
        value = roundDivU64(abs_ms, u64(MS_PER_MIN));
        if (value == 1) {
            suffix = " minute";
        } else {
            suffix = " minutes";
        }
    } else if (abs_ms >= u64(MS_PER_SEC)) {
        value = roundDivU64(abs_ms, u64(MS_PER_SEC));
        if (value == 1) {
            suffix = " second";
        } else {
            suffix = " seconds";
        }
    }

    let pos: usize = 0;
    if (is_neg && value > 0) {
        writeByte(out, *pos, '-')?;
    }
    writeU64(out, *pos, value)?;
    writeBytes(out, *pos, suffix)?;
    return out[0..pos];
}

fn appendNsSegment(out: []u8, pos: *usize, value: u64, unit: []const u8): void ! MsError {
    if (value == 0) {
        return;
    }
    if (pos.* > 0) {
        writeByte(out, pos, ' ')?;
    }
    writeU64(out, pos, value)?;
    writeBytes(out, pos, unit)?;
}

/// Formats unsigned nanoseconds into an exact Rust `humantime`-compatible compound string
/// (e.g. `"1h 30m 15s"`, `"0s"`, `"2d 3h 4m 5s 6ms"`).
pub fn formatDurationNs(ns: u64, out: []u8): ([]const u8) ! MsError {
    let pos: usize = 0;
    if (ns == 0) {
        writeBytes(out, *pos, "0s")?;
        return out[0..pos];
    }

    let rem = ns;
    const days = rem / NS_PER_DAY;
    rem = rem % NS_PER_DAY;
    const hours = rem / NS_PER_HOUR;
    rem = rem % NS_PER_HOUR;
    const mins = rem / NS_PER_MIN;
    rem = rem % NS_PER_MIN;
    const secs = rem / NS_PER_SEC;
    rem = rem % NS_PER_SEC;
    const millis = rem / NS_PER_MS;
    rem = rem % NS_PER_MS;
    const micros = rem / NS_PER_US;
    const nanos = rem % NS_PER_US;

    appendNsSegment(out, *pos, days, "d")?;
    appendNsSegment(out, *pos, hours, "h")?;
    appendNsSegment(out, *pos, mins, "m")?;
    appendNsSegment(out, *pos, secs, "s")?;
    appendNsSegment(out, *pos, millis, "ms")?;
    appendNsSegment(out, *pos, micros, "us")?;
    appendNsSegment(out, *pos, nanos, "ns")?;

    return out[0..pos];
}
