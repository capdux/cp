// Source: Absorbed from /data/dev/bun/src/runtime/node/time_like.cp | Synced: 2026-11-22T15:21:12Z
// Zero-allocation Node/Bun `TimeLike` timestamp normalization for POSIX `utimensat` / `fs.utimes`.

/// Nanoseconds per second.
pub const TIMELIKE_NS_PER_S: i64 = 1_000_000_000;

/// Nanoseconds per millisecond.
pub const TIMELIKE_NS_PER_MS: i64 = 1_000_000;

/// Milliseconds per second.
pub const TIMELIKE_MS_PER_S: i64 = 1_000;

/// Linux `utimensat(2)` sentinel meaning "set timestamp to current kernel clock" (`(1 << 30) - 1`).
pub const UTIME_NOW_LINUX: i64 = 0x3fffffff;

/// Linux `utimensat(2)` sentinel meaning "leave timestamp unchanged" (`(1 << 30) - 2`).
pub const UTIME_OMIT_LINUX: i64 = 0x3ffffffe;

/// Error set for `TimeLikeSpec` parsing and conversion.
pub err TimeLikeError {
    NonFinite,
    InvalidNumericString,
    OutOfRange,
}

fn isFiniteF64(v: f64): bool {
    if (v != v) return false;
    const max_f: f64 = 1.7976931348623157e308;
    return v >= -max_f && v <= max_f;
}

fn floorF64(v: f64): i64 {
    const truncated: i64 = @intFromFloat(v);
    const as_f: f64 = @as(f64, @floatFromInt(truncated));
    if (v < as_f) {
        return truncated - 1;
    }
    return truncated;
}

fn trimAsciiSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

fn parseDecimalF64(raw: []const u8): f64 ! TimeLikeError {
    const s = trimAsciiSpaces(raw);
    if (s.len == 0) return err.InvalidNumericString;

    let idx: usize = 0;
    let negative = false;
    if (s[0] == '-') {
        negative = true;
        idx = 1;
    } else if (s[0] == '+') {
        idx = 1;
    }
    if (idx >= s.len) return err.InvalidNumericString;

    let int_part: f64 = 0.0;
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 0.1;
    let seen_dot = false;
    let seen_digit = false;

    while (idx < s.len) {
        const c = s[idx];
        if (c >= '0' && c <= '9') {
            seen_digit = true;
            const digit: f64 = @as(f64, @floatFromInt(u32(c - '0')));
            if (!seen_dot) {
                int_part = int_part * 10.0 + digit;
            } else {
                frac_part += digit * frac_scale;
                frac_scale *= 0.1;
            }
        } else if (c == '.') {
            if (seen_dot) return err.InvalidNumericString;
            seen_dot = true;
        } else {
            return err.InvalidNumericString;
        }
        idx += 1;
    }

    if (!seen_digit) return err.InvalidNumericString;
    let val = int_part + frac_part;
    if (!isFiniteF64(val)) return err.NonFinite;
    if (negative) val = -val;
    return val;
}

/// Normalized POSIX `timespec`-compatible file timestamp representation.
/// Supports normal `(sec, nsec)` values (`0 <= nsec < 1_000_000_000`) as well as
/// Linux `utimensat(2)` `UTIME_NOW` and `UTIME_OMIT` sentinels.
pub struct TimeLikeSpec {
    sec: i64 = 0,
    nsec: i64 = 0,

    /// Construct the Linux `UTIME_NOW` sentinel (sets file timestamp to current kernel time).
    static fn nowSentinel(): TimeLikeSpec {
        return TimeLikeSpec{
            sec: 0,
            nsec: UTIME_NOW_LINUX,
        };
    }

    /// Construct the Linux `UTIME_OMIT` sentinel (leaves file timestamp unmodified).
    static fn omitSentinel(): TimeLikeSpec {
        return TimeLikeSpec{
            sec: 0,
            nsec: UTIME_OMIT_LINUX,
        };
    }

    /// Convert Unix epoch seconds (`f64`) using Node/Bun `fs.utimes` semantics:
    /// - NaN / Infinity returns `err.NonFinite`
    /// - Negative seconds map to `UTIME_NOW` sentinel (matches Bun `time_like.cp` `fromJS`)
    /// - Non-negative seconds split into `(sec, nsec)` with `0 <= nsec < 1_000_000_000`.
    static fn fromSeconds(seconds: f64): TimeLikeSpec ! TimeLikeError {
        if (!isFiniteF64(seconds)) return err.NonFinite;
        if (seconds < 0.0) {
            return TimeLikeSpec.nowSentinel();
        }
        if (seconds >= 4.0e18) return err.OutOfRange;

        const sec_i: i64 = @intFromFloat(seconds);
        const frac: f64 = seconds - @as(f64, @floatFromInt(sec_i));
        let nsec_i: i64 = @intFromFloat(frac * 1_000_000_000.0);
        let final_sec = sec_i;
        if (nsec_i < 0) {
            nsec_i = 0;
        } else if (nsec_i >= TIMELIKE_NS_PER_S) {
            final_sec += 1;
            nsec_i -= TIMELIKE_NS_PER_S;
        }
        return TimeLikeSpec{
            sec: final_sec,
            nsec: nsec_i,
        };
    }

    /// Convert arbitrary Unix epoch seconds (including pre-1970 negative timestamps)
    /// into normalized `(sec, nsec)` with `0 <= nsec < 1_000_000_000`.
    static fn fromUnixSeconds(seconds: f64): TimeLikeSpec ! TimeLikeError {
        if (!isFiniteF64(seconds)) return err.NonFinite;
        if (seconds <= -4.0e18 || seconds >= 4.0e18) return err.OutOfRange;

        let sec_i = floorF64(seconds);
        const frac: f64 = seconds - @as(f64, @floatFromInt(sec_i));
        let nsec_i: i64 = @intFromFloat(frac * 1_000_000_000.0);
        if (nsec_i < 0) {
            nsec_i += TIMELIKE_NS_PER_S;
            sec_i -= 1;
        } else if (nsec_i >= TIMELIKE_NS_PER_S) {
            nsec_i -= TIMELIKE_NS_PER_S;
            sec_i += 1;
        }
        return TimeLikeSpec{
            sec: sec_i,
            nsec: nsec_i,
        };
    }

    /// Convert Unix epoch milliseconds (e.g. `Date.getTime()`) into normalized `(sec, nsec)`
    /// with `0 <= nsec < 1_000_000_000`, faithfully matching Bun `fromMilliseconds`.
    static fn fromMilliseconds(milliseconds: f64): TimeLikeSpec ! TimeLikeError {
        if (!isFiniteF64(milliseconds)) return err.NonFinite;
        const seconds = milliseconds / 1000.0;
        return TimeLikeSpec.fromUnixSeconds(seconds);
    }

    /// Parse a numeric string representing Unix epoch seconds (e.g. `"123456789.5"`).
    /// Follows Node/Bun `fs.utimes` semantics (negative values map to `UTIME_NOW`).
    static fn fromNumericString(raw: []const u8): TimeLikeSpec ! TimeLikeError {
        const seconds = parseDecimalF64(raw)?;
        return TimeLikeSpec.fromSeconds(seconds);
    }

    fn isNow(): bool {
        return _.nsec == UTIME_NOW_LINUX;
    }

    fn isOmit(): bool {
        return _.nsec == UTIME_OMIT_LINUX;
    }

    fn isSpecial(): bool {
        return _.isNow() || _.isOmit();
    }

    /// Convert back to floating-point Unix seconds (`null` for `UTIME_NOW` / `UTIME_OMIT`).
    fn toFloatSeconds(): f64? {
        if (_.isSpecial()) return null;
        return @as(f64, @floatFromInt(_.sec)) + (@as(f64, @floatFromInt(_.nsec)) / 1_000_000_000.0);
    }

    /// Convert back to integer Unix milliseconds (`null` for `UTIME_NOW` / `UTIME_OMIT`).
    fn toUnixMillis(): i64? {
        if (_.isSpecial()) return null;
        return _.sec * TIMELIKE_MS_PER_S + (_.nsec / TIMELIKE_NS_PER_MS);
    }

    fn eql(other: TimeLikeSpec): bool {
        return _.sec == other.sec && _.nsec == other.nsec;
    }
}
