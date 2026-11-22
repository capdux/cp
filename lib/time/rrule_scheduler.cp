// Origin: Pure-CP Stdlib (Zero-Alloc RFC 5545 RRULE Parser, Formatter & Pure-Integer Occurrence Scheduler) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 5545 Recurrence Rule (RRULE) parser, canonical formatter,
// and pure-integer Gregorian next-occurrence / window-expansion scheduler:
//   - Parses & formats FREQ (DAILY|WEEKLY|MONTHLY|YEARLY), INTERVAL, COUNT, UNTIL (YYYYMMDDTHHMMSSZ),
//     BYDAY, BYMONTHDAY, BYMONTH, BYHOUR, BYMINUTE, and WKST with strict RFC 5545 validation
//   - Pure integer Howard Hinnant civil calendar arithmetic (zero float drift; exact leap-year Feb 29
//     and 31st-day month skipping)
//   - Period-bucket expansion for `nextOccurrence` and `expandOccurrences` respecting COUNT, UNTIL,
//     and hard iteration step caps

/// Maximum period iteration steps before halting search to prevent infinite loops on impossible rules.
pub const RRULE_MAX_ITER_STEPS: usize = 200_000;

const SECS_PER_MIN: i64 = 60;
const SECS_PER_HOUR: i64 = 3600;
const SECS_PER_DAY: i64 = 86400;

// ============================================================================
// 1. Public Enums, Civil Calendar Helpers & RRULE Model
// ============================================================================

/// Recurrence frequency (`FREQ`).
pub enum RRuleFreq {
    daily,
    weekly,
    monthly,
    yearly,
}

/// ISO-8601 weekday (`MO = 0` .. `SU = 6`).
pub enum RRuleWeekday {
    mo,
    tu,
    we,
    th,
    fr,
    sa,
    su,
}

/// Pure integer UTC Gregorian date-time tuple.
pub struct RRuleDateTime {
    year: i32 = 1970;
    month: u8 = 1;
    day: u8 = 1;
    hour: u8 = 0;
    minute: u8 = 0;
    second: u8 = 0;
}

/// Parsed RFC 5545 Recurrence Rule (`RRULE`) with compact zero-alloc bitmasks.
pub struct RRule {
    freq: RRuleFreq = RRuleFreq.daily;
    interval: u32 = 1;
    has_count: bool = false;
    count: u32 = 0;
    has_until: bool = false;
    until_ts: i64 = 0;
    /// Bit `0..6` for `MO..SU` (`0` = unspecified).
    by_day_mask: u8 = 0;
    /// Bit `1..12` for Jan..Dec (`0` = unspecified).
    by_month_mask: u16 = 0;
    /// Bit `1..31` for day `1..31` (`0` = unspecified).
    by_monthday_mask: u32 = 0;
    /// Bit `0..23` for hour `0..23` (`0` = unspecified).
    by_hour_mask: u32 = 0;
    /// Bit `0..59` for minute `0..59` (`0` = unspecified).
    by_minute_mask: u64 = 0;
    wkst: RRuleWeekday = RRuleWeekday.mo;
    has_wkst: bool = false;

    /// Returns the first occurrence timestamp strictly greater than `after_ts` starting from `dtstart`,
    /// or `null` if the recurrence terminates (COUNT/UNTIL) or hits `RRULE_MAX_ITER_STEPS`.
    fn nextOccurrence(dtstart: i64, after_ts: i64): i64? {
        return rruleNextOccurrence(_, dtstart, after_ts);
    }

    /// Expands all occurrence timestamps within `[window_start, window_end]` (inclusive) into `out_ts`.
    /// Returns the number of timestamps written.
    fn expandOccurrences(
        dtstart: i64,
        window_start: i64,
        window_end: i64,
        out_ts: *[]i64,
    ): usize {
        return rruleExpandOccurrences(_, dtstart, window_start, window_end, out_ts);
    }
}

fn weekdayToU8(wd: RRuleWeekday): u8 {
    if (wd == RRuleWeekday.mo) return 0;
    if (wd == RRuleWeekday.tu) return 1;
    if (wd == RRuleWeekday.we) return 2;
    if (wd == RRuleWeekday.th) return 3;
    if (wd == RRuleWeekday.fr) return 4;
    if (wd == RRuleWeekday.sa) return 5;
    return 6;
}

fn weekdayFromU8(v: u8): RRuleWeekday {
    if (v == 0) return RRuleWeekday.mo;
    if (v == 1) return RRuleWeekday.tu;
    if (v == 2) return RRuleWeekday.we;
    if (v == 3) return RRuleWeekday.th;
    if (v == 4) return RRuleWeekday.fr;
    if (v == 5) return RRuleWeekday.sa;
    return RRuleWeekday.su;
}

/// Returns `true` iff `year` is a Gregorian leap year.
pub fn isLeapYear(year: i32): bool {
    return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

/// Returns the number of days in `(year, month)` (`1..12`), or `0` for invalid month.
pub fn daysInMonth(year: i32, month: u8): u8 {
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        return 31;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    if (month == 2) {
        return if (isLeapYear(year)) 29 else 28;
    }
    return 0;
}

// Howard Hinnant's civil-from-days / days-from-civil pure integer algorithms.
fn civilToDays(year: i32, month: u8, day: u8): i64 {
    let y: i64 = year as i64;
    const m: i64 = month as i64;
    const d: i64 = day as i64;
    if (m <= 2) {
        y -= 1;
    }
    const era: i64 = if (y >= 0) (y / 400) else ((y - 399) / 400);
    const yoe: i64 = y - era * 400; // [0, 399]
    const mp: i64 = if (m > 2) (m - 3) else (m + 9); // [0, 11]
    const doy: i64 = (153 * mp + 2) / 5 + d - 1; // [0, 365]
    const doe: i64 = yoe * 365 + yoe / 4 - yoe / 100 + doy; // [0, 146096]
    return era * 146097 + doe - 719468;
}

fn daysToCivil(z_in: i64): RRuleDateTime {
    const z: i64 = z_in + 719468;
    const era: i64 = if (z >= 0) (z / 146097) else ((z - 146096) / 146097);
    const doe: i64 = z - era * 146097;
    const yoe: i64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y: i64 = yoe + era * 400;
    const doy: i64 = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const mp: i64 = (5 * doy + 2) / 153;
    const d: i64 = doy - (153 * mp + 2) / 5 + 1;
    const m: i64 = if (mp < 10) (mp + 3) else (mp - 9);
    if (m <= 2) {
        y += 1;
    }
    return RRuleDateTime{
        year: y as i32,
        month: m as u8,
        day: d as u8,
        hour: 0,
        minute: 0,
        second: 0,
    };
}

fn floorDiv(a: i64, b: i64): i64 {
    const q: i64 = a / b;
    const r: i64 = a % b;
    if (r < 0) return q - 1;
    return q;
}

fn floorMod(a: i64, b: i64): i64 {
    const r: i64 = a % b;
    if (r < 0) return r + b;
    return r;
}

/// Converts a validated UTC `RRuleDateTime` into Unix epoch seconds, or `null` on invalid date/time.
pub fn rruleDateTimeToUnix(dt: RRuleDateTime): i64? {
    if (dt.month < 1 || dt.month > 12) return null;
    const dim: u8 = daysInMonth(dt.year, dt.month);
    if (dt.day < 1 || dt.day > dim) return null;
    if (dt.hour > 23 || dt.minute > 59 || dt.second > 59) return null;
    const days: i64 = civilToDays(dt.year, dt.month, dt.day);
    return days * SECS_PER_DAY + (dt.hour as i64) * SECS_PER_HOUR + (dt.minute as i64) * SECS_PER_MIN + (dt.second as i64);
}

/// Converts Unix epoch seconds into a UTC `RRuleDateTime`.
pub fn rruleUnixToDateTime(ts: i64): RRuleDateTime {
    const days: i64 = floorDiv(ts, SECS_PER_DAY);
    const rem: i64 = floorMod(ts, SECS_PER_DAY);
    let dt = daysToCivil(days);
    dt.hour = (rem / SECS_PER_HOUR) as u8;
    dt.minute = ((rem % SECS_PER_HOUR) / SECS_PER_MIN) as u8;
    dt.second = (rem % SECS_PER_MIN) as u8;
    return dt;
}

/// Returns ISO-8601 weekday (`MO..SU`) for Unix epoch seconds `ts` (1970-01-01 = `TH`).
pub fn rruleWeekdayOfUnix(ts: i64): RRuleWeekday {
    const days: i64 = floorDiv(ts, SECS_PER_DAY);
    return weekdayFromU8(floorMod(days + 3, 7) as u8);
}

fn weekdayOfDays(days: i64): u8 {
    return floorMod(days + 3, 7) as u8;
}

// ============================================================================
// 2. RFC 5545 RRULE Parser & Canonical Formatter
// ============================================================================

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn parseUintDec(s: []const u8): u32? {
    if (s.len == 0 || s.len > 10) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch: u8 = s[i];
        if (ch < 48 || ch > 57) return null;
        acc = acc * 10 + ((ch - 48) as u64);
        if (acc > 0xFFFFFFFF) return null;
    }
    return acc as u32;
}

fn parseFixedDigits(s: []const u8, start: usize, len: usize): u32? {
    if (start + len > s.len) return null;
    return parseUintDec(s[start .. start + len]);
}

/// Parses an RFC 5545 UTC timestamp `YYYYMMDDTHHMMSSZ` (16 bytes) into Unix epoch seconds.
pub fn parseRRuleUtcTimestamp(s: []const u8): i64? {
    if (s.len != 16) return null;
    if (s[8] != 84 || s[15] != 90) return null; // 'T' and 'Z'
    const y = parseFixedDigits(s, 0, 4) ?? return null;
    const mo = parseFixedDigits(s, 4, 2) ?? return null;
    const d = parseFixedDigits(s, 6, 2) ?? return null;
    const h = parseFixedDigits(s, 9, 2) ?? return null;
    const mi = parseFixedDigits(s, 11, 2) ?? return null;
    const sec = parseFixedDigits(s, 13, 2) ?? return null;
    return rruleDateTimeToUnix(RRuleDateTime{
        year: y as i32,
        month: mo as u8,
        day: d as u8,
        hour: h as u8,
        minute: mi as u8,
        second: sec as u8,
    });
}

fn parseWeekdayToken(s: []const u8): RRuleWeekday? {
    if (eqBytes(s, "MO")) return RRuleWeekday.mo;
    if (eqBytes(s, "TU")) return RRuleWeekday.tu;
    if (eqBytes(s, "WE")) return RRuleWeekday.we;
    if (eqBytes(s, "TH")) return RRuleWeekday.th;
    if (eqBytes(s, "FR")) return RRuleWeekday.fr;
    if (eqBytes(s, "SA")) return RRuleWeekday.sa;
    if (eqBytes(s, "SU")) return RRuleWeekday.su;
    return null;
}

/// Parses an RFC 5545 `RRULE` string (with optional `"RRULE:"` prefix).
/// Enforces required `FREQ`, `INTERVAL >= 1`, mutual exclusion of `COUNT` and `UNTIL`,
/// no duplicate keys, and strict value range validation.
pub fn parseRRule(input: []const u8): RRule? {
    if (input.len == 0) return null;

    let src: []const u8 = input;
    if (src.len >= 6 && eqBytes(src[0..6], "RRULE:")) {
        src = src[6..];
    }
    if (src.len == 0) return null;

    let rule = RRule{};
    let seen_freq: bool = false;
    let seen_interval: bool = false;
    let seen_byday: bool = false;
    let seen_bymonth: bool = false;
    let seen_bymonthday: bool = false;
    let seen_byhour: bool = false;
    let seen_byminute: bool = false;
    let seen_wkst: bool = false;

    let pos: usize = 0;
    while (pos < src.len) {
        let seg_end: usize = pos;
        while (seg_end < src.len && src[seg_end] != 59) : (seg_end += 1) {} // ';'
        const seg: []const u8 = src[pos..seg_end];
        if (seg.len == 0) return null;

        let eq_pos: usize = 0;
        while (eq_pos < seg.len && seg[eq_pos] != 61) : (eq_pos += 1) {} // '='
        if (eq_pos == 0 || eq_pos + 1 >= seg.len) return null;

        const key: []const u8 = seg[0..eq_pos];
        const val: []const u8 = seg[eq_pos + 1 ..];

        if (eqBytes(key, "FREQ")) {
            if (seen_freq) return null;
            seen_freq = true;
            if (eqBytes(val, "DAILY")) {
                rule.freq = RRuleFreq.daily;
            } else if (eqBytes(val, "WEEKLY")) {
                rule.freq = RRuleFreq.weekly;
            } else if (eqBytes(val, "MONTHLY")) {
                rule.freq = RRuleFreq.monthly;
            } else if (eqBytes(val, "YEARLY")) {
                rule.freq = RRuleFreq.yearly;
            } else {
                return null;
            }
        } else if (eqBytes(key, "INTERVAL")) {
            if (seen_interval) return null;
            seen_interval = true;
            const n = parseUintDec(val) ?? return null;
            if (n == 0) return null;
            rule.interval = n;
        } else if (eqBytes(key, "COUNT")) {
            if (rule.has_count || rule.has_until) return null;
            const n = parseUintDec(val) ?? return null;
            if (n == 0) return null;
            rule.has_count = true;
            rule.count = n;
        } else if (eqBytes(key, "UNTIL")) {
            if (rule.has_until || rule.has_count) return null;
            const ts = parseRRuleUtcTimestamp(val) ?? return null;
            rule.has_until = true;
            rule.until_ts = ts;
        } else if (eqBytes(key, "WKST")) {
            if (seen_wkst) return null;
            seen_wkst = true;
            const wd = parseWeekdayToken(val) ?? return null;
            rule.wkst = wd;
            rule.has_wkst = true;
        } else if (eqBytes(key, "BYDAY")) {
            if (seen_byday) return null;
            seen_byday = true;
            let p: usize = 0;
            while (p < val.len) {
                let e: usize = p;
                while (e < val.len && val[e] != 44) : (e += 1) {} // ','
                if (e == p) return null;
                const wd = parseWeekdayToken(val[p..e]) ?? return null;
                rule.by_day_mask = rule.by_day_mask | ((1 as u8) << (weekdayToU8(wd) as u3));
                if (e == val.len) break;
                p = e + 1;
                if (p == val.len) return null; // trailing comma
            }
        } else if (eqBytes(key, "BYMONTH")) {
            if (seen_bymonth) return null;
            seen_bymonth = true;
            let p: usize = 0;
            while (p < val.len) {
                let e: usize = p;
                while (e < val.len && val[e] != 44) : (e += 1) {}
                if (e == p) return null;
                const mo = parseUintDec(val[p..e]) ?? return null;
                if (mo < 1 || mo > 12) return null;
                rule.by_month_mask = rule.by_month_mask | ((1 as u16) << (mo as u4));
                if (e == val.len) break;
                p = e + 1;
                if (p == val.len) return null;
            }
        } else if (eqBytes(key, "BYMONTHDAY")) {
            if (seen_bymonthday) return null;
            seen_bymonthday = true;
            let p: usize = 0;
            while (p < val.len) {
                let e: usize = p;
                while (e < val.len && val[e] != 44) : (e += 1) {}
                if (e == p) return null;
                const md = parseUintDec(val[p..e]) ?? return null;
                if (md < 1 || md > 31) return null;
                rule.by_monthday_mask = rule.by_monthday_mask | ((1 as u32) << (md as u5));
                if (e == val.len) break;
                p = e + 1;
                if (p == val.len) return null;
            }
        } else if (eqBytes(key, "BYHOUR")) {
            if (seen_byhour) return null;
            seen_byhour = true;
            let p: usize = 0;
            while (p < val.len) {
                let e: usize = p;
                while (e < val.len && val[e] != 44) : (e += 1) {}
                if (e == p) return null;
                const hr = parseUintDec(val[p..e]) ?? return null;
                if (hr > 23) return null;
                rule.by_hour_mask = rule.by_hour_mask | ((1 as u32) << (hr as u5));
                if (e == val.len) break;
                p = e + 1;
                if (p == val.len) return null;
            }
        } else if (eqBytes(key, "BYMINUTE")) {
            if (seen_byminute) return null;
            seen_byminute = true;
            let p: usize = 0;
            while (p < val.len) {
                let e: usize = p;
                while (e < val.len && val[e] != 44) : (e += 1) {}
                if (e == p) return null;
                const mi = parseUintDec(val[p..e]) ?? return null;
                if (mi > 59) return null;
                rule.by_minute_mask = rule.by_minute_mask | ((1 as u64) << (mi as u6));
                if (e == val.len) break;
                p = e + 1;
                if (p == val.len) return null;
            }
        } else {
            return null;
        }

        if (seg_end == src.len) break;
        pos = seg_end + 1;
        if (pos == src.len) return null; // trailing ';'
    }

    if (!seen_freq) return null;
    return rule;
}

fn appendStr(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* > out.len || s.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn appendU32Dec(out: *[]u8, pos: *usize, v: u32): bool {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let cur: u32 = v;
    if (cur == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

fn appendPaddedDigits(out: *[]u8, pos: *usize, v: u32, width: usize): bool {
    if (pos.* > out.len || width > out.len - pos.*) return false;
    let cur: u32 = v;
    let i: usize = width;
    while (i > 0) : (i -= 1) {
        out[pos.* + i - 1] = (48 + (cur % 10)) as u8;
        cur /= 10;
    }
    pos.* += width;
    return true;
}

fn weekdayName(idx: u8): []const u8 {
    if (idx == 0) return "MO";
    if (idx == 1) return "TU";
    if (idx == 2) return "WE";
    if (idx == 3) return "TH";
    if (idx == 4) return "FR";
    if (idx == 5) return "SA";
    return "SU";
}

/// Formats `rule` into a canonical RFC 5545 `RRULE` string into `out_buf`.
/// Returns written byte count, or `null` if `rule` is invalid or `out_buf` is too small.
pub fn formatRRule(rule: RRule, out_buf: *[]u8): usize? {
    if (rule.interval == 0 || (rule.has_count && rule.has_until)) return null;
    let pos: usize = 0;

    if (!appendStr(out_buf, *pos, "FREQ=")) return null;
    if (rule.freq == RRuleFreq.daily) {
        if (!appendStr(out_buf, *pos, "DAILY")) return null;
    } else if (rule.freq == RRuleFreq.weekly) {
        if (!appendStr(out_buf, *pos, "WEEKLY")) return null;
    } else if (rule.freq == RRuleFreq.monthly) {
        if (!appendStr(out_buf, *pos, "MONTHLY")) return null;
    } else {
        if (!appendStr(out_buf, *pos, "YEARLY")) return null;
    }

    if (rule.interval != 1) {
        if (!appendStr(out_buf, *pos, ";INTERVAL=")) return null;
        if (!appendU32Dec(out_buf, *pos, rule.interval)) return null;
    }
    if (rule.has_count) {
        if (!appendStr(out_buf, *pos, ";COUNT=")) return null;
        if (!appendU32Dec(out_buf, *pos, rule.count)) return null;
    }
    if (rule.has_until) {
        const udt = rruleUnixToDateTime(rule.until_ts);
        if (udt.year < 0 || udt.year > 9999) return null;
        if (!appendStr(out_buf, *pos, ";UNTIL=")) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.year as u32, 4)) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.month as u32, 2)) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.day as u32, 2)) return null;
        if (!appendStr(out_buf, *pos, "T")) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.hour as u32, 2)) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.minute as u32, 2)) return null;
        if (!appendPaddedDigits(out_buf, *pos, udt.second as u32, 2)) return null;
        if (!appendStr(out_buf, *pos, "Z")) return null;
    }
    if (rule.by_month_mask != 0) {
        if (!appendStr(out_buf, *pos, ";BYMONTH=")) return null;
        let first: bool = true;
        let m: u32 = 1;
        while (m <= 12) : (m += 1) {
            if (((rule.by_month_mask >> (m as u4)) & 1) != 0) {
                if (!first && !appendStr(out_buf, *pos, ",")) return null;
                if (!appendU32Dec(out_buf, *pos, m)) return null;
                first = false;
            }
        }
    }
    if (rule.by_monthday_mask != 0) {
        if (!appendStr(out_buf, *pos, ";BYMONTHDAY=")) return null;
        let first: bool = true;
        let d: u32 = 1;
        while (d <= 31) : (d += 1) {
            if (((rule.by_monthday_mask >> (d as u5)) & 1) != 0) {
                if (!first && !appendStr(out_buf, *pos, ",")) return null;
                if (!appendU32Dec(out_buf, *pos, d)) return null;
                first = false;
            }
        }
    }
    if (rule.by_day_mask != 0) {
        if (!appendStr(out_buf, *pos, ";BYDAY=")) return null;
        let first: bool = true;
        let wd: u8 = 0;
        while (wd < 7) : (wd += 1) {
            if (((rule.by_day_mask >> (wd as u3)) & 1) != 0) {
                if (!first && !appendStr(out_buf, *pos, ",")) return null;
                if (!appendStr(out_buf, *pos, weekdayName(wd))) return null;
                first = false;
            }
        }
    }
    if (rule.by_hour_mask != 0) {
        if (!appendStr(out_buf, *pos, ";BYHOUR=")) return null;
        let first: bool = true;
        let h: u32 = 0;
        while (h < 24) : (h += 1) {
            if (((rule.by_hour_mask >> (h as u5)) & 1) != 0) {
                if (!first && !appendStr(out_buf, *pos, ",")) return null;
                if (!appendU32Dec(out_buf, *pos, h)) return null;
                first = false;
            }
        }
    }
    if (rule.by_minute_mask != 0) {
        if (!appendStr(out_buf, *pos, ";BYMINUTE=")) return null;
        let first: bool = true;
        let mi: u32 = 0;
        while (mi < 60) : (mi += 1) {
            if (((rule.by_minute_mask >> (mi as u6)) & 1) != 0) {
                if (!first && !appendStr(out_buf, *pos, ",")) return null;
                if (!appendU32Dec(out_buf, *pos, mi)) return null;
                first = false;
            }
        }
    }
    if (rule.has_wkst) {
        if (!appendStr(out_buf, *pos, ";WKST=")) return null;
        if (!appendStr(out_buf, *pos, weekdayName(weekdayToU8(rule.wkst)))) return null;
    }

    return pos;
}

// ============================================================================
// 3. Period-Bucket Occurrence Expansion Engine
// ============================================================================

struct ExpandState {
    dtstart: i64 = 0;
    s_dt: RRuleDateTime = RRuleDateTime{},
    stop_after_first_gt: bool = false;
    after_ts: i64 = 0;
    window_start: i64 = 0;
    window_end: i64 = 0;
    emitted_count: u32 = 0;
    found_single: bool = false;
    single_ts: i64 = 0;
    out_written: usize = 0;
    done: bool = false;
}

fn emitDayTimes(
    rule: RRule,
    day_epoch: i64,
    st: *ExpandState,
    out_ts: *[]i64,
) {
    const h_mask: u32 = if (rule.by_hour_mask != 0) rule.by_hour_mask else ((1 as u32) << (st.s_dt.hour as u5));
    const m_mask: u64 = if (rule.by_minute_mask != 0) rule.by_minute_mask else ((1 as u64) << (st.s_dt.minute as u6));
    const base_sec: i64 = day_epoch * SECS_PER_DAY + (st.s_dt.second as i64);

    let h: u32 = 0;
    while (h < 24 && !st.done) : (h += 1) {
        if (((h_mask >> (h as u5)) & 1) == 0) continue;
        const h_sec: i64 = base_sec + (h as i64) * SECS_PER_HOUR;

        let mi: u32 = 0;
        while (mi < 60 && !st.done) : (mi += 1) {
            if (((m_mask >> (mi as u6)) & 1) == 0) continue;
            const cand_ts: i64 = h_sec + (mi as i64) * SECS_PER_MIN;

            if (cand_ts < st.dtstart) continue;
            if (rule.has_until && cand_ts > rule.until_ts) {
                st.done = true;
                return;
            }

            st.emitted_count += 1;
            if (rule.has_count && st.emitted_count > rule.count) {
                st.done = true;
                return;
            }

            if (st.stop_after_first_gt) {
                if (cand_ts > st.after_ts) {
                    st.found_single = true;
                    st.single_ts = cand_ts;
                    st.done = true;
                    return;
                }
            } else {
                if (cand_ts > st.window_end) {
                    st.done = true;
                    return;
                }
                if (cand_ts >= st.window_start) {
                    if (st.out_written < out_ts.len) {
                        out_ts[st.out_written] = cand_ts;
                        st.out_written += 1;
                    }
                    if (st.out_written >= out_ts.len) {
                        st.done = true;
                        return;
                    }
                }
            }
        }
    }
}

fn dayMatchesFilters(rule: RRule, month: u8, day: u8, wd: u8): bool {
    if (rule.by_month_mask != 0 && ((rule.by_month_mask >> (month as u4)) & 1) == 0) return false;
    if (rule.by_monthday_mask != 0 && ((rule.by_monthday_mask >> (day as u5)) & 1) == 0) return false;
    if (rule.by_day_mask != 0 && ((rule.by_day_mask >> (wd as u3)) & 1) == 0) return false;
    return true;
}

fn runExpansionLoop(rule: RRule, st: *ExpandState, out_ts: *[]i64) {
    if (rule.interval == 0 || (rule.has_count && rule.has_until)) return;
    const start_days: i64 = floorDiv(st.dtstart, SECS_PER_DAY);
    const start_wd: u8 = weekdayOfDays(start_days);
    const wkst_u8: u8 = weekdayToU8(rule.wkst);
    const offset_from_wkst: i64 = floorMod((start_wd as i64) - (wkst_u8 as i64), 7);
    const week0_start_days: i64 = start_days - offset_from_wkst;
    const interval_i64: i64 = rule.interval as i64;

    let step: usize = 0;
    while (step < RRULE_MAX_ITER_STEPS && !st.done) : (step += 1) {
        const p_idx: i64 = step as i64;

        if (rule.freq == RRuleFreq.daily) {
            const cur_day: i64 = start_days + p_idx * interval_i64;
            const cur_min_ts: i64 = cur_day * SECS_PER_DAY;
            if (rule.has_until && cur_min_ts > rule.until_ts) break;
            if (!st.stop_after_first_gt && cur_min_ts > st.window_end) break;

            const cdt = daysToCivil(cur_day);
            const wd: u8 = weekdayOfDays(cur_day);
            if (dayMatchesFilters(rule, cdt.month, cdt.day, wd)) {
                emitDayTimes(rule, cur_day, st, out_ts);
            }
        } else if (rule.freq == RRuleFreq.weekly) {
            const w_start: i64 = week0_start_days + p_idx * interval_i64 * 7;
            const w_min_ts: i64 = w_start * SECS_PER_DAY;
            if (rule.has_until && w_min_ts > rule.until_ts) break;
            if (!st.stop_after_first_gt && w_min_ts > st.window_end) break;

            const eff_byday: u8 = if (rule.by_day_mask != 0) rule.by_day_mask else ((1 as u8) << (start_wd as u3));
            let d_off: i64 = 0;
            while (d_off < 7 && !st.done) : (d_off += 1) {
                const cur_day: i64 = w_start + d_off;
                const wd: u8 = weekdayOfDays(cur_day);
                if (((eff_byday >> (wd as u3)) & 1) == 0) continue;
                const cdt = daysToCivil(cur_day);
                if (rule.by_month_mask != 0 && ((rule.by_month_mask >> (cdt.month as u4)) & 1) == 0) continue;
                if (rule.by_monthday_mask != 0 && ((rule.by_monthday_mask >> (cdt.day as u5)) & 1) == 0) continue;
                emitDayTimes(rule, cur_day, st, out_ts);
            }
        } else if (rule.freq == RRuleFreq.monthly) {
            const total_months: i64 = ((st.s_dt.year as i64) * 12 + ((st.s_dt.month - 1) as i64)) + p_idx * interval_i64;
            const cur_year: i32 = floorDiv(total_months, 12) as i32;
            const cur_month: u8 = (floorMod(total_months, 12) + 1) as u8;
            const m_start_days: i64 = civilToDays(cur_year, cur_month, 1);
            const m_min_ts: i64 = m_start_days * SECS_PER_DAY;
            if (rule.has_until && m_min_ts > rule.until_ts) break;
            if (!st.stop_after_first_gt && m_min_ts > st.window_end) break;

            if (rule.by_month_mask != 0 && ((rule.by_month_mask >> (cur_month as u4)) & 1) == 0) {
                continue;
            }
            const dim: u8 = daysInMonth(cur_year, cur_month);
            const use_default_md: bool = rule.by_monthday_mask == 0 && rule.by_day_mask == 0;

            let day_num: u8 = 1;
            while (day_num <= dim && !st.done) : (day_num += 1) {
                if (use_default_md) {
                    if (day_num != st.s_dt.day) continue;
                } else {
                    if (rule.by_monthday_mask != 0 && ((rule.by_monthday_mask >> (day_num as u5)) & 1) == 0) continue;
                    const wd: u8 = weekdayOfDays(m_start_days + ((day_num - 1) as i64));
                    if (rule.by_day_mask != 0 && ((rule.by_day_mask >> (wd as u3)) & 1) == 0) continue;
                }
                emitDayTimes(rule, m_start_days + ((day_num - 1) as i64), st, out_ts);
            }
        } else {
            // YEARLY
            const cur_year: i32 = st.s_dt.year + ((p_idx * interval_i64) as i32);
            const y_start_days: i64 = civilToDays(cur_year, 1, 1);
            const y_min_ts: i64 = y_start_days * SECS_PER_DAY;
            if (rule.has_until && y_min_ts > rule.until_ts) break;
            if (!st.stop_after_first_gt && y_min_ts > st.window_end) break;

            const eff_months: u16 = if (rule.by_month_mask != 0)
                rule.by_month_mask
            else
                ((1 as u16) << (st.s_dt.month as u4));
            const use_default_md: bool = rule.by_monthday_mask == 0 && rule.by_day_mask == 0;

            let mo: u8 = 1;
            while (mo <= 12 && !st.done) : (mo += 1) {
                if (((eff_months >> (mo as u4)) & 1) == 0) continue;
                const dim: u8 = daysInMonth(cur_year, mo);
                const m_start_days: i64 = civilToDays(cur_year, mo, 1);

                let day_num: u8 = 1;
                while (day_num <= dim && !st.done) : (day_num += 1) {
                    if (use_default_md) {
                        if (day_num != st.s_dt.day) continue;
                    } else {
                        if (rule.by_monthday_mask != 0 && ((rule.by_monthday_mask >> (day_num as u5)) & 1) == 0) continue;
                        const wd: u8 = weekdayOfDays(m_start_days + ((day_num - 1) as i64));
                        if (rule.by_day_mask != 0 && ((rule.by_day_mask >> (wd as u3)) & 1) == 0) continue;
                    }
                    emitDayTimes(rule, m_start_days + ((day_num - 1) as i64), st, out_ts);
                }
            }
        }
    }
}

/// Computes the next occurrence timestamp strictly greater than `after_ts` for `rule` anchored at `dtstart`.
pub fn rruleNextOccurrence(rule: RRule, dtstart: i64, after_ts: i64): i64? {
    let st = ExpandState{
        dtstart: dtstart,
        s_dt: rruleUnixToDateTime(dtstart),
        stop_after_first_gt: true,
        after_ts: after_ts,
    };
    let dummy: [1]i64 = [0; 1];
    let dummy_s: []i64 = dummy[0..];
    runExpansionLoop(rule, *st, *dummy_s);
    if (st.found_single) return st.single_ts;
    return null;
}

/// Expands all occurrences of `rule` anchored at `dtstart` falling in `[window_start, window_end]` into `out_ts`.
pub fn rruleExpandOccurrences(
    rule: RRule,
    dtstart: i64,
    window_start: i64,
    window_end: i64,
    out_ts: *[]i64,
): usize {
    if (out_ts.len == 0 || window_end < window_start) return 0;
    let st = ExpandState{
        dtstart: dtstart,
        s_dt: rruleUnixToDateTime(dtstart),
        stop_after_first_gt: false,
        window_start: window_start,
        window_end: window_end,
    };
    runExpansionLoop(rule, *st, out_ts);
    return st.out_written;
}
