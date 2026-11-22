// Source: cp/lib/intl/units.cp (SI/IEC Byte Size, Human Duration & Relative Time Codec)
// Pure CP bidirectional parser & formatter for SI/IEC byte sizes ("1.5 GiB", "256MB"),
// compound human durations ("1d 2h 30m 15s", "450ms"), and relative timestamps ("5 minutes ago").

const std = @import("std");

/// Error set for unit parsing and formatting (`ByteSize`, `HumanDuration`, `RelativeTime`).
pub err UnitError {
    InvalidFormat,
    Overflow,
    NegativeValue,
    OutOfMemory,
}

const U64_MAX_U128: u128 = std.math.maxInt(u64) as u128;

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn trimAscii(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) : (start += 1) {}
    while (end > start && isAsciiSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
    }
    return true;
}

fn appendU64(alloc: std.mem.Allocator, out: *std.ArrayList(u8), val: u64): void ! UnitError {
    if (val == 0) {
        out.append(alloc, '0') catch return UnitError.OutOfMemory;
    } else {
        let tmp: [32]u8 = undefined;
        let tlen: usize = 0;
        let v = val;
        while (v > 0) {
            tmp[tlen] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            tlen += 1;
        }
        let i = tlen;
        while (i > 0) {
            i -= 1;
            out.append(alloc, tmp[i]) catch return UnitError.OutOfMemory;
        }
    }
}

struct ParsedDecimal {
    int_part: u128 = 0;
    frac_num: u128 = 0;
    frac_den: u128 = 1;
    end_idx: usize = 0;
}

fn parsePositiveDecimalAt(s: string, start: usize): ParsedDecimal ! UnitError {
    let i = start;
    const n = s.len;
    if (i >= n) return UnitError.InvalidFormat;
    if (s[i] == '-') return UnitError.NegativeValue;
    if (s[i] == '+') return UnitError.InvalidFormat;

    if (!isAsciiDigit(s[i])) return UnitError.InvalidFormat;

    let int_part: u128 = 0;
    while (i < n && isAsciiDigit(s[i])) : (i += 1) {
        const d = (s[i] - '0') as u128;
        if (int_part > (U64_MAX_U128 - d) / 10) {
            // Allow digits to finish scanning so trailing invalid chars still trigger InvalidFormat vs Overflow
            let j = i;
            while (j < n && (isAsciiDigit(s[j]) || s[j] == '.')) : (j += 1) {}
            return UnitError.Overflow;
        }
        int_part = int_part * 10 + d;
    }

    let frac_num: u128 = 0;
    let frac_den: u128 = 1;
    if (i < n && s[i] == '.') {
        i += 1;
        if (i >= n || !isAsciiDigit(s[i])) return UnitError.InvalidFormat;
        let frac_digits: usize = 0;
        while (i < n && isAsciiDigit(s[i])) : (i += 1) {
            if (frac_digits < 18) {
                frac_num = frac_num * 10 + ((s[i] - '0') as u128);
                frac_den *= 10;
                frac_digits += 1;
            }
        }
    }

    return ParsedDecimal {
        int_part: int_part,
        frac_num: frac_num,
        frac_den: frac_den,
        end_idx: i,
    };
}

fn scaleDecimalToU64(parsed: ParsedDecimal, mult: u128): u64 ! UnitError {
    if (mult == 0) return 0;
    if (parsed.int_part > U64_MAX_U128 / mult) return UnitError.Overflow;
    const base = parsed.int_part * mult;
    const frac_scaled = (parsed.frac_num * mult + (parsed.frac_den / 2)) / parsed.frac_den;
    if (base > U64_MAX_U128 - frac_scaled) return UnitError.Overflow;
    const total = base + frac_scaled;
    if (total > U64_MAX_U128) return UnitError.Overflow;
    return total as u64;
}

fn formatScaledBytes(
    alloc: std.mem.Allocator,
    bytes: u64,
    base_u64: u64,
    units: []const []const u8,
): ([]u8) ! UnitError {
    if (bytes < base_u64) {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        appendU64(alloc, *out, bytes)?;
        out.appendSlice(alloc, " B") catch return UnitError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return UnitError.OutOfMemory;
    }

    const base: f64 = @floatFromInt(base_u64);
    let val: f64 = @floatFromInt(bytes);
    let idx: usize = 0;
    while (val >= base && idx + 1 < units.len) {
        val /= base;
        idx += 1;
    }

    // Round to 2 decimal places and promote unit if rounding pushes value to `base` (e.g. 999.996 KB -> 1 MB).
    let hundredths = @round(val * 100.0);
    const limit_hundredths = base * 100.0;
    if (idx + 1 < units.len && hundredths >= limit_hundredths) {
        val /= base;
        idx += 1;
        hundredths = @round(val * 100.0);
    }

    const h_u64: u64 = @intFromFloat(hundredths);
    const whole = h_u64 / 100;
    const frac = (h_u64 % 100) as u8;

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);
    appendU64(alloc, *out, whole)?;
    if (frac != 0) {
        out.append(alloc, '.') catch return UnitError.OutOfMemory;
        const d1 = frac / 10;
        const d2 = frac % 10;
        out.append(alloc, ('0' as u8) + d1) catch return UnitError.OutOfMemory;
        if (d2 != 0) {
            out.append(alloc, ('0' as u8) + d2) catch return UnitError.OutOfMemory;
        }
    }
    out.append(alloc, ' ') catch return UnitError.OutOfMemory;
    out.appendSlice(alloc, units[idx]) catch return UnitError.OutOfMemory;
    return out.toOwnedSlice(alloc) catch return UnitError.OutOfMemory;
}

fn byteUnitMultiplier(unit: string): u128? {
    if (unit.len == 0) return 1;
    if (asciiEqlIgnoreCase(unit, "b") || asciiEqlIgnoreCase(unit, "byte") || asciiEqlIgnoreCase(unit, "bytes")) return 1;

    // SI (base 1000)
    if (asciiEqlIgnoreCase(unit, "k") || asciiEqlIgnoreCase(unit, "kb")) return 1000;
    if (asciiEqlIgnoreCase(unit, "m") || asciiEqlIgnoreCase(unit, "mb")) return 1000000;
    if (asciiEqlIgnoreCase(unit, "g") || asciiEqlIgnoreCase(unit, "gb")) return 1000000000;
    if (asciiEqlIgnoreCase(unit, "t") || asciiEqlIgnoreCase(unit, "tb")) return 1000000000000;
    if (asciiEqlIgnoreCase(unit, "p") || asciiEqlIgnoreCase(unit, "pb")) return 1000000000000000;
    if (asciiEqlIgnoreCase(unit, "e") || asciiEqlIgnoreCase(unit, "eb")) return 1000000000000000000;

    // IEC (base 1024)
    if (asciiEqlIgnoreCase(unit, "ki") || asciiEqlIgnoreCase(unit, "kib")) return 1024;
    if (asciiEqlIgnoreCase(unit, "mi") || asciiEqlIgnoreCase(unit, "mib")) return 1048576;
    if (asciiEqlIgnoreCase(unit, "gi") || asciiEqlIgnoreCase(unit, "gib")) return 1073741824;
    if (asciiEqlIgnoreCase(unit, "ti") || asciiEqlIgnoreCase(unit, "tib")) return 1099511627776;
    if (asciiEqlIgnoreCase(unit, "pi") || asciiEqlIgnoreCase(unit, "pib")) return 1125899906842624;
    if (asciiEqlIgnoreCase(unit, "ei") || asciiEqlIgnoreCase(unit, "eib")) return 1152921504606846976;

    return null;
}

fn durationUnitMultiplierMs(unit: string): u128? {
    if (asciiEqlIgnoreCase(unit, "ms") || asciiEqlIgnoreCase(unit, "msec") || asciiEqlIgnoreCase(unit, "msecs")) return 1;
    if (asciiEqlIgnoreCase(unit, "s") || asciiEqlIgnoreCase(unit, "sec") || asciiEqlIgnoreCase(unit, "secs") || asciiEqlIgnoreCase(unit, "second") || asciiEqlIgnoreCase(unit, "seconds")) return 1000;
    if (asciiEqlIgnoreCase(unit, "m") || asciiEqlIgnoreCase(unit, "min") || asciiEqlIgnoreCase(unit, "mins") || asciiEqlIgnoreCase(unit, "minute") || asciiEqlIgnoreCase(unit, "minutes")) return 60000;
    if (asciiEqlIgnoreCase(unit, "h") || asciiEqlIgnoreCase(unit, "hr") || asciiEqlIgnoreCase(unit, "hrs") || asciiEqlIgnoreCase(unit, "hour") || asciiEqlIgnoreCase(unit, "hours")) return 3600000;
    if (asciiEqlIgnoreCase(unit, "d") || asciiEqlIgnoreCase(unit, "day") || asciiEqlIgnoreCase(unit, "days")) return 86400000;
    if (asciiEqlIgnoreCase(unit, "w") || asciiEqlIgnoreCase(unit, "wk") || asciiEqlIgnoreCase(unit, "wks") || asciiEqlIgnoreCase(unit, "week") || asciiEqlIgnoreCase(unit, "weeks")) return 604800000;
    return null;
}

/// Bidirectional SI (base-1000) and IEC (base-1024) byte size formatter and parser.
pub static struct ByteSize {
    /// Formats `bytes` using SI decimal prefixes (`B`, `KB`, `MB`, `GB`, `TB`, `PB`).
    static fn formatSi(alloc: std.mem.Allocator, bytes: u64): ([]u8) ! UnitError {
        const si_units: [6][]const u8 = ["B", "KB", "MB", "GB", "TB", "PB"];
        return formatScaledBytes(alloc, bytes, 1000, &si_units);
    }

    /// Formats `bytes` using IEC binary prefixes (`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`).
    static fn formatIec(alloc: std.mem.Allocator, bytes: u64): ([]u8) ! UnitError {
        const iec_units: [6][]const u8 = ["B", "KiB", "MiB", "GiB", "TiB", "PiB"];
        return formatScaledBytes(alloc, bytes, 1024, &iec_units);
    }

    /// Case-insensitively parses byte size strings (`"1.5 GiB"`, `"256MB"`, `"512 k"`, `"1024"`) with overflow & negative guards.
    static fn parse(str: string): u64 ! UnitError {
        const s = trimAscii(str);
        if (s.len == 0) return UnitError.InvalidFormat;

        const parsed = parsePositiveDecimalAt(s, 0)?;
        let pos = parsed.end_idx;
        while (pos < s.len && isAsciiSpace(s[pos])) : (pos += 1) {}

        const unit_start = pos;
        while (pos < s.len && isAsciiAlpha(s[pos])) : (pos += 1) {}
        if (pos != s.len) return UnitError.InvalidFormat;

        const unit = s[unit_start..pos];
        const mult_opt = byteUnitMultiplier(unit);
        if (mult_opt == null) return UnitError.InvalidFormat;
        const mult = mult_opt.?;
        return scaleDecimalToU64(parsed, mult);
    }
}

fn appendDurationPart(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    val: u64,
    suffix: string,
): void ! UnitError {
    if (val > 0) {
        if (out.items.len > 0) {
            out.append(alloc, ' ') catch return UnitError.OutOfMemory;
        }
        appendU64(alloc, out, val)?;
        out.appendSlice(alloc, suffix) catch return UnitError.OutOfMemory;
    }
}

/// Bidirectional compound human-readable duration formatter and parser (`"1d 2h 30m 15s"`, `"450ms"`).
pub static struct HumanDuration {
    /// Formats milliseconds into compound human-readable segments (`"1d 2h 30m 15s"`, `"450ms"`).
    static fn formatMs(alloc: std.mem.Allocator, ms: u64): ([]u8) ! UnitError {
        if (ms == 0) {
            return alloc.dupe(u8, "0ms") catch return UnitError.OutOfMemory;
        }

        const days = ms / 86400000;
        let rem = ms % 86400000;
        const hours = rem / 3600000;
        rem %= 3600000;
        const mins = rem / 60000;
        rem %= 60000;
        const secs = rem / 1000;
        const millis = rem % 1000;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        appendDurationPart(alloc, *out, days, "d")?;
        appendDurationPart(alloc, *out, hours, "h")?;
        appendDurationPart(alloc, *out, mins, "m")?;
        appendDurationPart(alloc, *out, secs, "s")?;
        appendDurationPart(alloc, *out, millis, "ms")?;
        return out.toOwnedSlice(alloc) catch return UnitError.OutOfMemory;
    }

    /// Parses compound durations (`"1h30m15s"`, `"2d 4h"`, `"500ms"`, `"1.5h"`) into milliseconds with overflow protection.
    static fn parseMs(str: string): u64 ! UnitError {
        const s = trimAscii(str);
        if (s.len == 0) return UnitError.InvalidFormat;
        if (s[0] == '-') return UnitError.NegativeValue;

        let pos: usize = 0;
        let total_ms: u128 = 0;
        let segments: usize = 0;

        while (pos < s.len) {
            while (pos < s.len && isAsciiSpace(s[pos])) : (pos += 1) {}
            if (pos >= s.len) break;

            const parsed = parsePositiveDecimalAt(s, pos)?;
            pos = parsed.end_idx;

            while (pos < s.len && isAsciiSpace(s[pos])) : (pos += 1) {}
            const unit_start = pos;
            while (pos < s.len && isAsciiAlpha(s[pos])) : (pos += 1) {}
            if (pos == unit_start) return UnitError.InvalidFormat;

            const unit = s[unit_start..pos];
            const mult_opt = durationUnitMultiplierMs(unit);
            if (mult_opt == null) return UnitError.InvalidFormat;
            const mult = mult_opt.?;
            const part_ms = scaleDecimalToU64(parsed, mult)?;

            total_ms += part_ms as u128;
            if (total_ms > U64_MAX_U128) return UnitError.Overflow;
            segments += 1;
        }

        if (segments == 0) return UnitError.InvalidFormat;
        return total_ms as u64;
    }
}

/// Human-friendly relative timestamp formatter with exact English singular/plural grammar.
pub static struct RelativeTime {
    /// Formats signed delta seconds (`< 0` past `"5 minutes ago"`, `> 0` future `"in 2 hours"`, `|delta| < 5` `"just now"`).
    static fn formatDeltaSec(alloc: std.mem.Allocator, delta_sec: i64): ([]u8) ! UnitError {
        const is_past = delta_sec < 0;
        let abs_sec: u64 = 0;
        if (is_past) {
            const raw: u64 = @bitCast(delta_sec);
            abs_sec = (0 as u64) -% raw;
        } else {
            abs_sec = delta_sec as u64;
        }

        if (abs_sec < 5) {
            return alloc.dupe(u8, "just now") catch return UnitError.OutOfMemory;
        }

        let count: u64 = abs_sec;
        let singular: string = "second";
        let plural: string = "seconds";

        if (abs_sec < 60) {
            count = abs_sec;
            singular = "second";
            plural = "seconds";
        } else if (abs_sec < 3600) {
            count = abs_sec / 60;
            singular = "minute";
            plural = "minutes";
        } else if (abs_sec < 86400) {
            count = abs_sec / 3600;
            singular = "hour";
            plural = "hours";
        } else if (abs_sec < 2592000) {
            count = abs_sec / 86400;
            singular = "day";
            plural = "days";
        } else if (abs_sec < 31536000) {
            count = abs_sec / 2592000;
            singular = "month";
            plural = "months";
        } else {
            count = abs_sec / 31536000;
            singular = "year";
            plural = "years";
        }

        const unit_word = if (count == 1) singular else plural;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        if (!is_past) {
            out.appendSlice(alloc, "in ") catch return UnitError.OutOfMemory;
        }
        appendU64(alloc, *out, count)?;
        out.append(alloc, ' ') catch return UnitError.OutOfMemory;
        out.appendSlice(alloc, unit_word) catch return UnitError.OutOfMemory;
        if (is_past) {
            out.appendSlice(alloc, " ago") catch return UnitError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return UnitError.OutOfMemory;
    }
}
