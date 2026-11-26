// packages/@cap/ms-parse/src/index.cp
// Zero-allocation human-readable duration string parser & formatter (npm `ms` parity).
// MIT clean-room pure CP implementation.

pub const MS_SECOND: i64 = 1000;
pub const MS_MINUTE: i64 = 60000;
pub const MS_HOUR: i64 = 3600000;
pub const MS_DAY: i64 = 86400000;
pub const MS_WEEK: i64 = 604800000;
pub const MS_YEAR: i64 = 31557600000;

/// Result of parsing a human-readable duration string into milliseconds.
pub struct MsResult {
    pub ms: i64,
    pub valid: bool,
}

fn mpToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn mpIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn mpTrim(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && mpIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && mpIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn mpEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (mpToLower(a[i]) != mpToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn mpUnitMultiplier(unit: []const u8): f64 {
    if (unit.len == 0 || mpEqIgnoreCase(unit, "ms") || mpEqIgnoreCase(unit, "msec") || mpEqIgnoreCase(unit, "msecs") || mpEqIgnoreCase(unit, "millisecond") || mpEqIgnoreCase(unit, "milliseconds")) {
        return 1.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "s") || mpEqIgnoreCase(unit, "sec") || mpEqIgnoreCase(unit, "secs") || mpEqIgnoreCase(unit, "second") || mpEqIgnoreCase(unit, "seconds")) {
        return 1000.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "m") || mpEqIgnoreCase(unit, "min") || mpEqIgnoreCase(unit, "mins") || mpEqIgnoreCase(unit, "minute") || mpEqIgnoreCase(unit, "minutes")) {
        return 60000.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "h") || mpEqIgnoreCase(unit, "hr") || mpEqIgnoreCase(unit, "hrs") || mpEqIgnoreCase(unit, "hour") || mpEqIgnoreCase(unit, "hours")) {
        return 3600000.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "d") || mpEqIgnoreCase(unit, "day") || mpEqIgnoreCase(unit, "days")) {
        return 86400000.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "w") || mpEqIgnoreCase(unit, "week") || mpEqIgnoreCase(unit, "weeks")) {
        return 604800000.0 as f64;
    }
    if (mpEqIgnoreCase(unit, "y") || mpEqIgnoreCase(unit, "yr") || mpEqIgnoreCase(unit, "yrs") || mpEqIgnoreCase(unit, "year") || mpEqIgnoreCase(unit, "years")) {
        return 31557600000.0 as f64;
    }
    return -1.0 as f64;
}

/// Parse a human-readable duration string (e.g. `"2 days"`, `"1.5h"`, `"-3s"`, `"500ms"`, `"1w"`, `"42"`)
/// into signed milliseconds (`i64`). Rejects empty strings, strings > 100 bytes, and unknown units.
pub fn parseMs(input: string): MsResult {
    let invalid = MsResult{
        ms: 0 as i64,
        valid: false,
    };

    const t = mpTrim(input);
    if (t.len == 0 || t.len > 100) {
        return invalid;
    }

    let i: usize = 0;
    let neg = false;
    if (t[0] == ('-' as u8)) {
        neg = true;
        i = 1;
    } else if (t[0] == ('+' as u8)) {
        i = 1;
    }
    if (i >= t.len) {
        return invalid;
    }

    let intPart: f64 = 0.0;
    let fracPart: f64 = 0.0;
    let digits: usize = 0;

    while (i < t.len && t[i] >= ('0' as u8) && t[i] <= ('9' as u8)) {
        intPart = intPart * (10.0 as f64) + ((t[i] - ('0' as u8)) as f64);
        digits += 1;
        i += 1;
    }

    if (i < t.len && t[i] == ('.' as u8)) {
        i += 1;
        let scale: f64 = 0.1;
        let fracDigits: usize = 0;
        while (i < t.len && t[i] >= ('0' as u8) && t[i] <= ('9' as u8)) {
            fracPart = fracPart + ((t[i] - ('0' as u8)) as f64) * scale;
            scale = scale * (0.1 as f64);
            digits += 1;
            fracDigits += 1;
            i += 1;
        }
        if (fracDigits == 0) {
            return invalid;
        }
    }

    if (digits == 0) {
        return invalid;
    }

    // Skip optional spaces between numeric value and unit.
    while (i < t.len && mpIsSpace(t[i])) {
        i += 1;
    }

    const unitSlice = t[i..t.len];
    const mult: f64 = mpUnitMultiplier(unitSlice);
    if (mult < (0.0 as f64)) {
        return invalid;
    }

    const total: f64 = (intPart + fracPart) * mult;
    if (total > (9000000000000000000.0 as f64)) {
        return invalid;
    }

    const rounded: i64 = ((total + (0.5 as f64)) as u64) as i64;
    if (neg) {
        return MsResult{ ms: -rounded, valid: true };
    }
    return MsResult{ ms: rounded, valid: true };
}

fn mpWriteRaw(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn mpWriteU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == (0 as u64)) {
        return mpWriteRaw(out, pos, "0");
    }
    let rev: [24]u8 = undefined;
    let n: usize = 0;
    let cur = val;
    while (cur > (0 as u64)) {
        rev[n] = ('0' as u8) + ((cur % (10 as u64)) as u8);
        n += 1;
        cur = cur / (10 as u64);
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.*] = rev[n - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn mpAbsU64(ms: i64): u64 {
    if (ms >= (0 as i64)) {
        return ms as u64;
    }
    return ((-(ms + (1 as i64))) as u64) + (1 as u64);
}

fn mpFormatImpl(ms: i64, longFmt: bool, out: []u8): usize {
    const neg = ms < (0 as i64);
    const absMs = mpAbsU64(ms);

    let unitDiv: u64 = 1 as u64;
    let shortSuffix: []const u8 = "ms";
    let longSing: []const u8 = " ms";
    let longPlur: []const u8 = " ms";

    if (absMs >= (86400000 as u64)) {
        unitDiv = 86400000 as u64;
        shortSuffix = "d";
        longSing = " day";
        longPlur = " days";
    } else if (absMs >= (3600000 as u64)) {
        unitDiv = 3600000 as u64;
        shortSuffix = "h";
        longSing = " hour";
        longPlur = " hours";
    } else if (absMs >= (60000 as u64)) {
        unitDiv = 60000 as u64;
        shortSuffix = "m";
        longSing = " minute";
        longPlur = " minutes";
    } else if (absMs >= (1000 as u64)) {
        unitDiv = 1000 as u64;
        shortSuffix = "s";
        longSing = " second";
        longPlur = " seconds";
    }

    const rounded: u64 = (absMs + (unitDiv / (2 as u64))) / unitDiv;

    let pos: usize = 0;
    if (neg && rounded > (0 as u64)) {
        if (!mpWriteRaw(out, &pos, "-")) {
            return 0;
        }
    }
    if (!mpWriteU64(out, &pos, rounded)) {
        return 0;
    }

    if (longFmt) {
        if (rounded == (1 as u64)) {
            if (!mpWriteRaw(out, &pos, longSing)) {
                return 0;
            }
        } else {
            if (!mpWriteRaw(out, &pos, longPlur)) {
                return 0;
            }
        }
    } else {
        if (!mpWriteRaw(out, &pos, shortSuffix)) {
            return 0;
        }
    }

    return pos;
}

/// Format milliseconds in short form (e.g. `60000` -> `"1m"`, `3600000` -> `"1h"`, `-5000` -> `"-5s"`).
/// Returns bytes written, or `0` if `out` is too small.
pub fn formatShort(ms: i64, out: []u8): usize {
    return mpFormatImpl(ms, false, out);
}

/// Format milliseconds in long form with singular/plural grammar
/// (e.g. `60000` -> `"1 minute"`, `120000` -> `"2 minutes"`).
/// Returns bytes written, or `0` if `out` is too small.
pub fn formatLong(ms: i64, out: []u8): usize {
    return mpFormatImpl(ms, true, out);
}

export {
    MS_SECOND,
    MS_MINUTE,
    MS_HOUR,
    MS_DAY,
    MS_WEEK,
    MS_YEAR,
    MsResult,
    parseMs,
    formatShort,
    formatLong,
};
