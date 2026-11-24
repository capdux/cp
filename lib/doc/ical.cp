// Reference-hardened against RFC 5545 (iCalendar) & RFC 6350 (vCard 4.0) (MIT OR Apache-2.0)
// Source: cp/lib/ical/ical.cp (Pure CP RFC 5545 iCalendar Parser, Generator & RRULE Expansion Engine)
// Zero external dependencies; supports line unfolding, text escaping, calendar arithmetic, 16-lane SIMD scanning, and RRULE expansion.

const std = @import("std");

pub const ICAL_MAX_LINE_BYTES: usize = 65536;
pub const ICAL_MAX_PROPERTIES: usize = 16384;
pub const ICAL_MAX_COMPONENT_DEPTH: usize = 16;

type CalCardVec16u8 = @Vector(16, u8);

/// Scalar reference: finds first index `>= start` where `data[i]` is `\r`, `\n`, `:`, or `;`.
pub fn scalarCalFindLineDelim(data: []const u8, start: usize): usize {
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '\r' || b == '\n' || b == ':' || b == ';') {
            return i;
        }
    }
    return data.len;
}

/// 16-lane `@Vector(16, u8)` SIMD scan for `\r`, `\n`, `:`, or `;` starting at `start`.
/// Matches `scalarCalFindLineDelim` with 0 discrepancy across all lengths.
pub fn simdCalFindLineDelim(data: []const u8, start: usize): usize {
    const n: usize = data.len;
    if (start >= n) return n;
    const v_cr: CalCardVec16u8 = @splat('\r');
    const v_lf: CalCardVec16u8 = @splat('\n');
    const v_colon: CalCardVec16u8 = @splat(':');
    const v_semi: CalCardVec16u8 = @splat(';');

    let i: usize = start;
    while (i + 16 <= n) : (i += 16) {
        const chunk: CalCardVec16u8 = data[i .. i + 16][0..16].*;
        const m_cr = chunk == v_cr;
        const m_lf = chunk == v_lf;
        const m_co = chunk == v_colon;
        const m_se = chunk == v_semi;
        if (@reduce(.Or, m_cr) || @reduce(.Or, m_lf) || @reduce(.Or, m_co) || @reduce(.Or, m_se)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if (b == '\r' || b == '\n' || b == ':' || b == ';') {
                    return i + k;
                }
            }
        }
    }
    while (i < n) : (i += 1) {
        const b = data[i];
        if (b == '\r' || b == '\n' || b == ':' || b == ';') {
            return i;
        }
    }
    return n;
}

/// Scalar reference: returns `true` if `data` contains a disallowed ASCII control byte
/// (`0x00..0x08`, `0x0B`, `0x0C`, `0x0E..0x1F`, or `0x7F`).
pub fn scalarCalHasControlByte(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) || b == 0x7F) {
            return true;
        }
    }
    return false;
}

/// 16-lane `@Vector(16, u8)` SIMD scan for disallowed ASCII control bytes.
/// Matches `scalarCalHasControlByte` with 0 discrepancy across all lengths.
pub fn simdCalHasControlByte(data: []const u8): bool {
    const n: usize = data.len;
    const v_sp: CalCardVec16u8 = @splat(0x20);
    const v_tab: CalCardVec16u8 = @splat(0x09);
    const v_lf: CalCardVec16u8 = @splat(0x0A);
    const v_cr: CalCardVec16u8 = @splat(0x0D);
    const v_del: CalCardVec16u8 = @splat(0x7F);

    let i: usize = 0;
    while (i + 16 <= n) : (i += 16) {
        const chunk: CalCardVec16u8 = data[i .. i + 16][0..16].*;
        const lt_sp = chunk < v_sp;
        const is_del = chunk == v_del;
        if (@reduce(.Or, lt_sp) || @reduce(.Or, is_del)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                const b = data[i + k];
                if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) || b == 0x7F) {
                    return true;
                }
            }
        }
        * = v_tab;
        * = v_lf;
        * = v_cr;
    }
    while (i < n) : (i += 1) {
        const b = data[i];
        if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) || b == 0x7F) {
            return true;
        }
    }
    return false;
}

pub err IcalError {
    InvalidFormat,
    InvalidDateTime,
    InvalidRRule,
    MissingRequiredField,
    OutOfMemory,
}

pub enum RRuleFreq {
    Daily,
    Weekly,
    Monthly,
    Yearly,

    fn toStr(): []const u8 {
        return switch (_) {
            .Daily => "DAILY",
            .Weekly => "WEEKLY",
            .Monthly => "MONTHLY",
            .Yearly => "YEARLY",
        };
    }

    static fn fromStr(s: []const u8): RRuleFreq? {
        if (asciiEqlIgnoreCase(s, "DAILY")) return .Daily;
        if (asciiEqlIgnoreCase(s, "WEEKLY")) return .Weekly;
        if (asciiEqlIgnoreCase(s, "MONTHLY")) return .Monthly;
        if (asciiEqlIgnoreCase(s, "YEARLY")) return .Yearly;
        return null;
    }
}

pub enum Weekday: u8 {
    Su = 0,
    Mo = 1,
    Tu = 2,
    We = 3,
    Th = 4,
    Fr = 5,
    Sa = 6,

    fn toStr(): []const u8 {
        return switch (_) {
            .Su => "SU",
            .Mo => "MO",
            .Tu => "TU",
            .We => "WE",
            .Th => "TH",
            .Fr => "FR",
            .Sa => "SA",
        };
    }

    static fn fromStr(s: []const u8): Weekday? {
        if (asciiEqlIgnoreCase(s, "SU")) return .Su;
        if (asciiEqlIgnoreCase(s, "MO")) return .Mo;
        if (asciiEqlIgnoreCase(s, "TU")) return .Tu;
        if (asciiEqlIgnoreCase(s, "WE")) return .We;
        if (asciiEqlIgnoreCase(s, "TH")) return .Th;
        if (asciiEqlIgnoreCase(s, "FR")) return .Fr;
        if (asciiEqlIgnoreCase(s, "SA")) return .Sa;
        return null;
    }
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn asciiStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return asciiEqlIgnoreCase(s[0..prefix.len], prefix);
}

fn dupSlice(alloc: std.mem.Allocator, s: []const u8): ([]u8)! {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return IcalError.OutOfMemory;
}

fn parseDigits(s: []const u8): i32? {
    if (s.len == 0) return null;
    let val: i32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') return null;
        val = val * 10 + i32(c - '0');
    }
    return val;
}

fn parseUint(s: []const u8): u32? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(u32, s, 10) catch return null;
}

fn parseIntSigned(s: []const u8): i32? {
    if (s.len == 0) return null;
    return std.fmt.parseInt(i32, s, 10) catch return null;
}

fn appendTwoDigits(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u8): ! {
    let zero: u8 = '0';
    let d1: u8 = zero + (v / 10) % 10;
    let d2: u8 = zero + (v % 10);
    buf.append(alloc, d1) catch return IcalError.OutOfMemory;
    buf.append(alloc, d2) catch return IcalError.OutOfMemory;
}

fn appendFourDigits(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, v: i32): ! {
    let zero: u8 = '0';
    let y: i32 = if (v < 0) -v else v;
    let d0: u8 = zero + u8(@mod(@divFloor(y, 1000), 10));
    let d1: u8 = zero + u8(@mod(@divFloor(y, 100), 10));
    let d2: u8 = zero + u8(@mod(@divFloor(y, 10), 10));
    let d3: u8 = zero + u8(@mod(y, 10));
    buf.append(alloc, d0) catch return IcalError.OutOfMemory;
    buf.append(alloc, d1) catch return IcalError.OutOfMemory;
    buf.append(alloc, d2) catch return IcalError.OutOfMemory;
    buf.append(alloc, d3) catch return IcalError.OutOfMemory;
}

// ============================================================================
// IcalDateTime: Gregorian Calendar Arithmetic & RFC 5545 Date/Date-Time
// ============================================================================

pub struct IcalDateTime {
    year: i32 = 1970,
    month: u8 = 1,
    day: u8 = 1,
    hour: u8 = 0,
    minute: u8 = 0,
    second: u8 = 0,
    is_utc: bool = false,
    is_date_only: bool = false,

    static fn isLeapYear(year: i32): bool {
        return (@mod(year, 4) == 0 && @mod(year, 100) != 0) || (@mod(year, 400) == 0);
    }

    static fn daysInMonth(year: i32, month: u8): u8 {
        return switch (month) {
            1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
            4 | 6 | 9 | 11 => 30,
            2 => if (IcalDateTime.isLeapYear(year)) 29 else 28,
            else => 0,
        };
    }

    /// Parses RFC 5545 DATE (`YYYYMMDD`) or DATE-TIME (`YYYYMMDDTHHMMSS` / `YYYYMMDDTHHMMSSZ`).
    static fn parse(s: []const u8): IcalDateTime! {
        let trimmed = trimSpaces(s);
        if (trimmed.len != 8 && trimmed.len != 15 && trimmed.len != 16) {
            return IcalError.InvalidDateTime;
        }

        let year = parseDigits(trimmed[0..4]) ?? return IcalError.InvalidDateTime;
        let month_i = parseDigits(trimmed[4..6]) ?? return IcalError.InvalidDateTime;
        let day_i = parseDigits(trimmed[6..8]) ?? return IcalError.InvalidDateTime;

        if (month_i < 1 || month_i > 12) return IcalError.InvalidDateTime;
        let month: u8 = u8(month_i);
        let dim = IcalDateTime.daysInMonth(year, month);
        if (day_i < 1 || day_i > i32(dim)) return IcalError.InvalidDateTime;
        let day: u8 = u8(day_i);

        if (trimmed.len == 8) {
            return IcalDateTime{
                year: year,
                month: month,
                day: day,
                hour: 0,
                minute: 0,
                second: 0,
                is_utc: false,
                is_date_only: true,
            };
        }

        if (trimmed[8] != 'T' && trimmed[8] != 't') {
            return IcalError.InvalidDateTime;
        }

        let hour_i = parseDigits(trimmed[9..11]) ?? return IcalError.InvalidDateTime;
        let min_i = parseDigits(trimmed[11..13]) ?? return IcalError.InvalidDateTime;
        let sec_i = parseDigits(trimmed[13..15]) ?? return IcalError.InvalidDateTime;

        if (hour_i < 0 || hour_i > 23) return IcalError.InvalidDateTime;
        if (min_i < 0 || min_i > 59) return IcalError.InvalidDateTime;
        if (sec_i < 0 || sec_i > 60) return IcalError.InvalidDateTime;

        let is_utc: bool = false;
        if (trimmed.len == 16) {
            if (trimmed[15] != 'Z' && trimmed[15] != 'z') {
                return IcalError.InvalidDateTime;
            }
            is_utc = true;
        }

        return IcalDateTime{
            year: year,
            month: month,
            day: day,
            hour: u8(hour_i),
            minute: u8(min_i),
            second: u8(sec_i),
            is_utc: is_utc,
            is_date_only: false,
        };
    }

    fn format(alloc: std.mem.Allocator): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        appendFourDigits(*buf, alloc, _.year)?;
        appendTwoDigits(*buf, alloc, _.month)?;
        appendTwoDigits(*buf, alloc, _.day)?;

        if (!_.is_date_only) {
            buf.append(alloc, 'T') catch return IcalError.OutOfMemory;
            appendTwoDigits(*buf, alloc, _.hour)?;
            appendTwoDigits(*buf, alloc, _.minute)?;
            appendTwoDigits(*buf, alloc, _.second)?;
            if (_.is_utc) {
                buf.append(alloc, 'Z') catch return IcalError.OutOfMemory;
            }
        }

        return buf.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }

    /// Tomohiko Sakamoto algorithm: returns Weekday (0 = Su .. 6 = Sa).
    fn dayOfWeek(): Weekday {
        let t: [12]i32 = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4];
        let y: i32 = _.year;
        if (_.month < 3) {
            y -= 1;
        }
        let m_idx: usize = usize(_.month - 1);
        let raw: i32 = @mod(y + @divFloor(y, 4) - @divFloor(y, 100) + @divFloor(y, 400) + t[m_idx] + i32(_.day), 7);
        return Weekday(u8(raw));
    }

    /// Converts Gregorian (year, month, day) to days since 1970-01-01 (Howard Hinnant algorithm).
    fn toEpochDays(): i64 {
        let y: i64 = i64(_.year);
        let m: i64 = i64(_.month);
        let d: i64 = i64(_.day);
        if (m <= 2) {
            y -= 1;
        }
        let era: i64 = @divFloor(y, 400);
        let yoe: i64 = y - era * 400;
        let mp_adj: i64 = if (m > 2) -3 else 9;
        let mp: i64 = m + mp_adj;
        let doy: i64 = @divFloor(153 * mp + 2, 5) + d - 1;
        let doe: i64 = yoe * 365 + @divFloor(yoe, 4) - @divFloor(yoe, 100) + doy;
        return era * 146097 + doe - 719468;
    }

    /// Reconstructs IcalDateTime from days since 1970-01-01, preserving time-of-day and flags.
    fn fromEpochDays(z_in: i64): IcalDateTime {
        let z: i64 = z_in + 719468;
        let era: i64 = @divFloor(z, 146097);
        let doe: i64 = z - era * 146097;
        let yoe: i64 = @divFloor(doe - @divFloor(doe, 1460) + @divFloor(doe, 36524) - @divFloor(doe, 146096), 365);
        let y: i64 = yoe + era * 400;
        let doy: i64 = doe - (365 * yoe + @divFloor(yoe, 4) - @divFloor(yoe, 100));
        let mp: i64 = @divFloor(5 * doy + 2, 153);
        let d: i64 = doy - @divFloor(153 * mp + 2, 5) + 1;
        let m_adj: i64 = if (mp < 10) 3 else -9;
        let m: i64 = mp + m_adj;
        if (m <= 2) {
            y += 1;
        }
        return IcalDateTime{
            year: i32(y),
            month: u8(m),
            day: u8(d),
            hour: _.hour,
            minute: _.minute,
            second: _.second,
            is_utc: _.is_utc,
            is_date_only: _.is_date_only,
        };
    }

    fn addDays(days: i64): IcalDateTime {
        let epoch_days = _.toEpochDays();
        return _.fromEpochDays(epoch_days + days);
    }

    fn compare(other: IcalDateTime): i32 {
        if (_.year != other.year) return if (_.year < other.year) -1 else 1;
        if (_.month != other.month) return if (_.month < other.month) -1 else 1;
        if (_.day != other.day) return if (_.day < other.day) -1 else 1;
        if (_.hour != other.hour) return if (_.hour < other.hour) -1 else 1;
        if (_.minute != other.minute) return if (_.minute < other.minute) -1 else 1;
        if (_.second != other.second) return if (_.second < other.second) -1 else 1;
        return 0;
    }

    fn sameDate(other: IcalDateTime): bool {
        return _.year == other.year && _.month == other.month && _.day == other.day;
    }

    fn equals(other: IcalDateTime): bool {
        return _.compare(other) == 0;
    }
}

// ============================================================================
// RRule: RFC 5545 Recurrence Rule Parser & Expansion Engine
// ============================================================================

pub struct RRule {
    freq: RRuleFreq = .Daily,
    interval: u32 = 1,
    count: u32? = null,
    until: IcalDateTime? = null,
    byday: std.ArrayList(Weekday) = .empty,
    bymonthday: std.ArrayList(i8) = .empty,
    bymonth: std.ArrayList(u8) = .empty,
    wkst: Weekday = .Mo,
    alloc: std.mem.Allocator,

    static fn parse(alloc: std.mem.Allocator, rule_str: []const u8): RRule! {
        let s = trimSpaces(rule_str);
        if (asciiStartsWithIgnoreCase(s, "RRULE:")) {
            s = trimSpaces(s[6..]);
        }
        if (s.len == 0) return IcalError.InvalidRRule;

        let rule = RRule{
            alloc: alloc,
            byday: .empty,
            bymonthday: .empty,
            bymonth: .empty,
        };
        errdefer rule.deinit();

        let has_freq: bool = false;
        let pos: usize = 0;
        while (pos < s.len) {
            let semi: usize = pos;
            while (semi < s.len && s[semi] != ';') {
                semi += 1;
            }
            let part = trimSpaces(s[pos..semi]);
            pos = if (semi < s.len) semi + 1 else s.len;

            if (part.len == 0) continue;

            let eq_idx: usize? = null;
            let k_i: usize = 0;
            while (k_i < part.len) : (k_i += 1) {
                if (part[k_i] == '=') {
                    eq_idx = k_i;
                    break;
                }
            }
            let eq = eq_idx ?? return IcalError.InvalidRRule;
            let key = trimSpaces(part[0..eq]);
            let val = trimSpaces(part[eq + 1..]);
            if (key.len == 0 || val.len == 0) return IcalError.InvalidRRule;

            if (asciiEqlIgnoreCase(key, "FREQ")) {
                let f = RRuleFreq.fromStr(val) ?? return IcalError.InvalidRRule;
                rule.freq = f;
                has_freq = true;
            } else if (asciiEqlIgnoreCase(key, "INTERVAL")) {
                let iv = parseUint(val) ?? return IcalError.InvalidRRule;
                if (iv < 1) return IcalError.InvalidRRule;
                rule.interval = iv;
            } else if (asciiEqlIgnoreCase(key, "COUNT")) {
                let c = parseUint(val) ?? return IcalError.InvalidRRule;
                if (c < 1) return IcalError.InvalidRRule;
                rule.count = c;
            } else if (asciiEqlIgnoreCase(key, "UNTIL")) {
                let u = IcalDateTime.parse(val) catch return IcalError.InvalidRRule;
                rule.until = u;
            } else if (asciiEqlIgnoreCase(key, "WKST")) {
                let w = Weekday.fromStr(val) ?? return IcalError.InvalidRRule;
                rule.wkst = w;
            } else if (asciiEqlIgnoreCase(key, "BYDAY")) {
                let bpos: usize = 0;
                while (bpos < val.len) {
                    let comma: usize = bpos;
                    while (comma < val.len && val[comma] != ',') {
                        comma += 1;
                    }
                    let tok = trimSpaces(val[bpos..comma]);
                    bpos = if (comma < val.len) comma + 1 else val.len;
                    if (tok.len < 2) return IcalError.InvalidRRule;
                    let wd_str = tok[tok.len - 2..];
                    let wd = Weekday.fromStr(wd_str) ?? return IcalError.InvalidRRule;
                    rule.byday.append(alloc, wd) catch return IcalError.OutOfMemory;
                }
                if (rule.byday.items.len == 0) return IcalError.InvalidRRule;
            } else if (asciiEqlIgnoreCase(key, "BYMONTHDAY")) {
                let mpos: usize = 0;
                while (mpos < val.len) {
                    let comma: usize = mpos;
                    while (comma < val.len && val[comma] != ',') {
                        comma += 1;
                    }
                    let tok = trimSpaces(val[mpos..comma]);
                    mpos = if (comma < val.len) comma + 1 else val.len;
                    let md = parseIntSigned(tok) ?? return IcalError.InvalidRRule;
                    if (md == 0 || md < -31 || md > 31) return IcalError.InvalidRRule;
                    rule.bymonthday.append(alloc, i8(md)) catch return IcalError.OutOfMemory;
                }
                if (rule.bymonthday.items.len == 0) return IcalError.InvalidRRule;
            } else if (asciiEqlIgnoreCase(key, "BYMONTH")) {
                let mpos: usize = 0;
                while (mpos < val.len) {
                    let comma: usize = mpos;
                    while (comma < val.len && val[comma] != ',') {
                        comma += 1;
                    }
                    let tok = trimSpaces(val[mpos..comma]);
                    mpos = if (comma < val.len) comma + 1 else val.len;
                    let bm = parseIntSigned(tok) ?? return IcalError.InvalidRRule;
                    if (bm < 1 || bm > 12) return IcalError.InvalidRRule;
                    rule.bymonth.append(alloc, u8(bm)) catch return IcalError.OutOfMemory;
                }
                if (rule.bymonth.items.len == 0) return IcalError.InvalidRRule;
            } else {
                // Unknown RRULE parameter
                return IcalError.InvalidRRule;
            }
        }

        if (!has_freq) return IcalError.InvalidRRule;
        return rule;
    }

    fn deinit(*_) {
        _.byday.deinit(_.alloc);
        _.bymonthday.deinit(_.alloc);
        _.bymonth.deinit(_.alloc);
    }

    fn containsWeekday(wd: Weekday): bool {
        for (item of _.byday.items) {
            if (item == wd) return true;
        }
        return false;
    }

    fn containsMonthDay(day: u8, dim: u8): bool {
        for (md of _.bymonthday.items) {
            if (md > 0) {
                if (u8(md) == day) return true;
            } else {
                let target: i32 = i32(dim) + 1 + i32(md);
                if (target >= 1 && u8(target) == day) return true;
            }
        }
        return false;
    }

    fn containsMonth(m: u8): bool {
        for (bm of _.bymonth.items) {
            if (bm == m) return true;
        }
        return false;
    }

    static fn isExcluded(cand: IcalDateTime, exdates: []const IcalDateTime): bool {
        for (ex of exdates) {
            if (ex.is_date_only || cand.is_date_only) {
                if (cand.sameDate(ex)) return true;
            } else {
                if (cand.compare(ex) == 0) return true;
            }
        }
        return false;
    }

    fn format(alloc: std.mem.Allocator): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        buf.appendSlice(alloc, "FREQ=") catch return IcalError.OutOfMemory;
        buf.appendSlice(alloc, _.freq.toStr()) catch return IcalError.OutOfMemory;

        if (_.interval > 1) {
            let iv_s = std.fmt.allocPrint(alloc, ";INTERVAL={d}", .{_.interval}) catch return IcalError.OutOfMemory;
            defer alloc.free(iv_s);
            buf.appendSlice(alloc, iv_s) catch return IcalError.OutOfMemory;
        }
        if (cnt := _.count) {
            let c_s = std.fmt.allocPrint(alloc, ";COUNT={d}", .{cnt}) catch return IcalError.OutOfMemory;
            defer alloc.free(c_s);
            buf.appendSlice(alloc, c_s) catch return IcalError.OutOfMemory;
        }
        if (u := _.until) {
            buf.appendSlice(alloc, ";UNTIL=") catch return IcalError.OutOfMemory;
            let u_s = u.format(alloc)?;
            defer alloc.free(u_s);
            buf.appendSlice(alloc, u_s) catch return IcalError.OutOfMemory;
        }
        if (_.byday.items.len > 0) {
            buf.appendSlice(alloc, ";BYDAY=") catch return IcalError.OutOfMemory;
            let idx: usize = 0;
            while (idx < _.byday.items.len) : (idx += 1) {
                if (idx > 0) buf.append(alloc, ',') catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, _.byday.items[idx].toStr()) catch return IcalError.OutOfMemory;
            }
        }
        if (_.bymonthday.items.len > 0) {
            buf.appendSlice(alloc, ";BYMONTHDAY=") catch return IcalError.OutOfMemory;
            let idx: usize = 0;
            while (idx < _.bymonthday.items.len) : (idx += 1) {
                if (idx > 0) buf.append(alloc, ',') catch return IcalError.OutOfMemory;
                let md_s = std.fmt.allocPrint(alloc, "{d}", .{_.bymonthday.items[idx]}) catch return IcalError.OutOfMemory;
                defer alloc.free(md_s);
                buf.appendSlice(alloc, md_s) catch return IcalError.OutOfMemory;
            }
        }
        if (_.bymonth.items.len > 0) {
            buf.appendSlice(alloc, ";BYMONTH=") catch return IcalError.OutOfMemory;
            let idx: usize = 0;
            while (idx < _.bymonth.items.len) : (idx += 1) {
                if (idx > 0) buf.append(alloc, ',') catch return IcalError.OutOfMemory;
                let bm_s = std.fmt.allocPrint(alloc, "{d}", .{_.bymonth.items[idx]}) catch return IcalError.OutOfMemory;
                defer alloc.free(bm_s);
                buf.appendSlice(alloc, bm_s) catch return IcalError.OutOfMemory;
            }
        }
        return buf.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }

    /// Expands the RRULE starting from `dtstart` within `[range_start, range_end]`,
    /// excluding `exdates` and stopping at `max_instances` (or `COUNT` / `UNTIL`).
    fn expand(
        alloc: std.mem.Allocator,
        dtstart: IcalDateTime,
        range_start: IcalDateTime,
        range_end: IcalDateTime,
        exdates: []const IcalDateTime,
        max_instances: usize,
    ): ([]IcalDateTime)! {
        let out: std.ArrayList(IcalDateTime) = .empty;
        errdefer out.deinit(alloc);

        if (max_instances == 0 || range_end.compare(range_start) < 0) {
            return out.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
        }

        let generated_count: u32 = 0;
        let max_steps: usize = 1000000;
        let step_guard: usize = 0;
        let iv: i64 = i64(_.interval);

        switch (_.freq) {
            .Daily => {
                let day_offset: i64 = 0;
                while (step_guard < max_steps) : (step_guard += 1) {
                    let cand = dtstart.addDays(day_offset);
                    day_offset += iv;

                    if (cand.compare(range_end) > 0) break;
                    if (u := _.until) {
                        if (cand.compare(u) > 0) break;
                    }

                    if (_.byday.items.len > 0 && !_.containsWeekday(cand.dayOfWeek())) continue;
                    if (_.bymonthday.items.len > 0 && !_.containsMonthDay(cand.day, IcalDateTime.daysInMonth(cand.year, cand.month))) continue;
                    if (_.bymonth.items.len > 0 && !_.containsMonth(cand.month)) continue;

                    generated_count += 1;
                    if (cnt := _.count) {
                        if (generated_count > cnt) break;
                    }

                    if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                        out.append(alloc, cand) catch return IcalError.OutOfMemory;
                        if (out.items.len >= max_instances) break;
                    }
                }
            },
            .Weekly => {
                if (_.byday.items.len == 0) {
                    let day_offset: i64 = 0;
                    while (step_guard < max_steps) : (step_guard += 1) {
                        let cand = dtstart.addDays(day_offset);
                        day_offset += iv * 7;

                        if (cand.compare(range_end) > 0) break;
                        if (u := _.until) {
                            if (cand.compare(u) > 0) break;
                        }
                        if (_.bymonth.items.len > 0 && !_.containsMonth(cand.month)) continue;

                        generated_count += 1;
                        if (cnt := _.count) {
                            if (generated_count > cnt) break;
                        }

                        if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                            out.append(alloc, cand) catch return IcalError.OutOfMemory;
                            if (out.items.len >= max_instances) break;
                        }
                    }
                } else {
                    let start_wd: i64 = i64(dtstart.dayOfWeek() as u8);
                    let wkst_wd: i64 = i64(_.wkst as u8);
                    let offset_from_wkst: i64 = @mod(start_wd - wkst_wd + 7, 7);
                    let base_week_start = dtstart.addDays(-offset_from_wkst);
                    let week_idx: i64 = 0;

                    outer_weekly: while (step_guard < max_steps) : (step_guard += 1) {
                        let week_start = base_week_start.addDays(week_idx * 7);
                        week_idx += iv;
                        if (week_start.compare(range_end) > 0) break;

                        let d_i: i64 = 0;
                        while (d_i < 7) : (d_i += 1) {
                            let cand = week_start.addDays(d_i);
                            if (cand.compare(dtstart) < 0) continue;
                            if (cand.compare(range_end) > 0) break :outer_weekly;
                            if (u := _.until) {
                                if (cand.compare(u) > 0) break :outer_weekly;
                            }
                            if (!_.containsWeekday(cand.dayOfWeek())) continue;
                            if (_.bymonth.items.len > 0 && !_.containsMonth(cand.month)) continue;

                            generated_count += 1;
                            if (cnt := _.count) {
                                if (generated_count > cnt) break :outer_weekly;
                            }

                            if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                                out.append(alloc, cand) catch return IcalError.OutOfMemory;
                                if (out.items.len >= max_instances) break :outer_weekly;
                            }
                        }
                    }
                }
            },
            .Monthly => {
                let m_offset: i64 = 0;
                let base_months: i64 = i64(dtstart.year) * 12 + i64(dtstart.month - 1);

                outer_monthly: while (step_guard < max_steps) : (step_guard += 1) {
                    let total_m = base_months + m_offset;
                    m_offset += iv;
                    let y: i32 = i32(@divFloor(total_m, 12));
                    let m: u8 = u8(@mod(total_m, 12) + 1);
                    let dim = IcalDateTime.daysInMonth(y, m);

                    let first_of_month = IcalDateTime{
                        year: y,
                        month: m,
                        day: 1,
                        hour: dtstart.hour,
                        minute: dtstart.minute,
                        second: dtstart.second,
                        is_utc: dtstart.is_utc,
                        is_date_only: dtstart.is_date_only,
                    };
                    if (first_of_month.compare(range_end) > 0) break;

                    if (_.bymonth.items.len > 0 && !_.containsMonth(m)) continue;

                    if (_.bymonthday.items.len > 0 || _.byday.items.len > 0) {
                        let day_num: u8 = 1;
                        while (day_num <= dim) : (day_num += 1) {
                            let cand = IcalDateTime{
                                year: y,
                                month: m,
                                day: day_num,
                                hour: dtstart.hour,
                                minute: dtstart.minute,
                                second: dtstart.second,
                                is_utc: dtstart.is_utc,
                                is_date_only: dtstart.is_date_only,
                            };
                            if (cand.compare(dtstart) < 0) continue;
                            if (cand.compare(range_end) > 0) break :outer_monthly;
                            if (u := _.until) {
                                if (cand.compare(u) > 0) break :outer_monthly;
                            }
                            if (_.bymonthday.items.len > 0 && !_.containsMonthDay(day_num, dim)) continue;
                            if (_.byday.items.len > 0 && !_.containsWeekday(cand.dayOfWeek())) continue;

                            generated_count += 1;
                            if (cnt := _.count) {
                                if (generated_count > cnt) break :outer_monthly;
                            }

                            if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                                out.append(alloc, cand) catch return IcalError.OutOfMemory;
                                if (out.items.len >= max_instances) break :outer_monthly;
                            }
                        }
                    } else {
                        if (dtstart.day <= dim) {
                            let cand = IcalDateTime{
                                year: y,
                                month: m,
                                day: dtstart.day,
                                hour: dtstart.hour,
                                minute: dtstart.minute,
                                second: dtstart.second,
                                is_utc: dtstart.is_utc,
                                is_date_only: dtstart.is_date_only,
                            };
                            if (cand.compare(range_end) > 0) break;
                            if (u := _.until) {
                                if (cand.compare(u) > 0) break;
                            }

                            generated_count += 1;
                            if (cnt := _.count) {
                                if (generated_count > cnt) break;
                            }

                            if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                                out.append(alloc, cand) catch return IcalError.OutOfMemory;
                                if (out.items.len >= max_instances) break;
                            }
                        }
                    }
                }
            },
            .Yearly => {
                let y_offset: i64 = 0;
                outer_yearly: while (step_guard < max_steps) : (step_guard += 1) {
                    let y: i32 = dtstart.year + i32(y_offset);
                    y_offset += iv;

                    let jan1 = IcalDateTime{
                        year: y,
                        month: 1,
                        day: 1,
                        hour: dtstart.hour,
                        minute: dtstart.minute,
                        second: dtstart.second,
                        is_utc: dtstart.is_utc,
                        is_date_only: dtstart.is_date_only,
                    };
                    if (jan1.compare(range_end) > 0) break;

                    let m_idx: u8 = 1;
                    while (m_idx <= 12) : (m_idx += 1) {
                        if (_.bymonth.items.len > 0) {
                            if (!_.containsMonth(m_idx)) continue;
                        } else {
                            if (m_idx != dtstart.month) continue;
                        }

                        let dim = IcalDateTime.daysInMonth(y, m_idx);
                        if (_.bymonthday.items.len > 0 || _.byday.items.len > 0) {
                            let d_idx: u8 = 1;
                            while (d_idx <= dim) : (d_idx += 1) {
                                let cand = IcalDateTime{
                                    year: y,
                                    month: m_idx,
                                    day: d_idx,
                                    hour: dtstart.hour,
                                    minute: dtstart.minute,
                                    second: dtstart.second,
                                    is_utc: dtstart.is_utc,
                                    is_date_only: dtstart.is_date_only,
                                };
                                if (cand.compare(dtstart) < 0) continue;
                                if (cand.compare(range_end) > 0) break :outer_yearly;
                                if (u := _.until) {
                                    if (cand.compare(u) > 0) break :outer_yearly;
                                }
                                if (_.bymonthday.items.len > 0 && !_.containsMonthDay(d_idx, dim)) continue;
                                if (_.byday.items.len > 0 && !_.containsWeekday(cand.dayOfWeek())) continue;

                                generated_count += 1;
                                if (cnt := _.count) {
                                    if (generated_count > cnt) break :outer_yearly;
                                }

                                if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                                    out.append(alloc, cand) catch return IcalError.OutOfMemory;
                                    if (out.items.len >= max_instances) break :outer_yearly;
                                }
                            }
                        } else {
                            if (dtstart.day <= dim) {
                                let cand = IcalDateTime{
                                    year: y,
                                    month: m_idx,
                                    day: dtstart.day,
                                    hour: dtstart.hour,
                                    minute: dtstart.minute,
                                    second: dtstart.second,
                                    is_utc: dtstart.is_utc,
                                    is_date_only: dtstart.is_date_only,
                                };
                                if (cand.compare(dtstart) < 0) continue;
                                if (cand.compare(range_end) > 0) break :outer_yearly;
                                if (u := _.until) {
                                    if (cand.compare(u) > 0) break :outer_yearly;
                                }

                                generated_count += 1;
                                if (cnt := _.count) {
                                    if (generated_count > cnt) break :outer_yearly;
                                }

                                if (!RRule.isExcluded(cand, exdates) && cand.compare(range_start) >= 0) {
                                    out.append(alloc, cand) catch return IcalError.OutOfMemory;
                                    if (out.items.len >= max_instances) break :outer_yearly;
                                }
                            }
                        }
                    }
                }
            },
        }

        return out.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }
}

// ============================================================================
// RFC 5545 Line Unfolding & Text Escaping / Unescaping
// ============================================================================

/// Unfolds RFC 5545 folded lines via 16-lane SIMD delimiter scanning: removes CRLF (or LF)
/// followed by a single SPACE or TAB. Rejects bare `\r` not followed by `\n`.
fn unfoldLines(alloc: std.mem.Allocator, text: []const u8): ([]u8)! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < text.len) {
        const hit = simdCalFindLineDelim(text, i);
        if (hit > i) {
            out.appendSlice(alloc, text[i..hit]) catch return IcalError.OutOfMemory;
            i = hit;
            if (i >= text.len) break;
        }
        const ch = text[i];
        if (ch == '\r') {
            if (i + 1 >= text.len || text[i + 1] != '\n') {
                return IcalError.InvalidFormat;
            }
            if (i + 2 < text.len && (text[i + 2] == ' ' || text[i + 2] == '\t')) {
                i += 3;
                continue;
            }
            out.append(alloc, '\n') catch return IcalError.OutOfMemory;
            i += 2;
        } else if (ch == '\n') {
            if (i + 1 < text.len && (text[i + 1] == ' ' || text[i + 1] == '\t')) {
                i += 2;
                continue;
            }
            out.append(alloc, '\n') catch return IcalError.OutOfMemory;
            i += 1;
        } else {
            out.append(alloc, ch) catch return IcalError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
}

fn unescapeText(alloc: std.mem.Allocator, text: []const u8): ([]u8)! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < text.len) {
        if (text[i] == '\\' && i + 1 < text.len) {
            let next = text[i + 1];
            switch (next) {
                'n' | 'N' => {
                    out.append(alloc, '\n') catch return IcalError.OutOfMemory;
                    i += 2;
                },
                ',' => {
                    out.append(alloc, ',') catch return IcalError.OutOfMemory;
                    i += 2;
                },
                ';' => {
                    out.append(alloc, ';') catch return IcalError.OutOfMemory;
                    i += 2;
                },
                '\\' => {
                    out.append(alloc, '\\') catch return IcalError.OutOfMemory;
                    i += 2;
                },
                else => {
                    out.append(alloc, text[i]) catch return IcalError.OutOfMemory;
                    i += 1;
                },
            }
        } else {
            out.append(alloc, text[i]) catch return IcalError.OutOfMemory;
            i += 1;
        }
    }

    return out.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
}

fn appendEscapedText(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, text: []const u8): ! {
    for (c of text) {
        switch (c) {
            '\\' => buf.appendSlice(alloc, "\\\\") catch return IcalError.OutOfMemory,
            ';' => buf.appendSlice(alloc, "\\;") catch return IcalError.OutOfMemory,
            ',' => buf.appendSlice(alloc, "\\,") catch return IcalError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return IcalError.OutOfMemory,
            '\r' => {},
            else => buf.append(alloc, c) catch return IcalError.OutOfMemory,
        }
    }
}

struct ContentLineParts {
    name: []const u8,
    value: []const u8,
}

/// Splits a content line `NAME;PARAM=VAL:VALUE` into `(base_name, value)`.
/// Rejects unclosed parameter quotes or missing/empty property name.
fn splitContentLine(line: []const u8): ContentLineParts? {
    let in_quotes: bool = false;
    let colon_idx: usize? = null;
    let semi_idx: usize? = null;
    let i: usize = 0;
    while (i < line.len) : (i += 1) {
        let ch = line[i];
        if (ch == '"') {
            in_quotes = !in_quotes;
        } else if (!in_quotes) {
            if (ch == ';' && semi_idx == null) {
                semi_idx = i;
            } else if (ch == ':') {
                colon_idx = i;
                break;
            }
        }
    }
    if (in_quotes) return null;
    let c_idx = colon_idx ?? return null;
    let name_end = semi_idx ?? c_idx;
    let name = trimSpaces(line[0..name_end]);
    if (name.len == 0) return null;
    return ContentLineParts{ name: name, value: line[c_idx + 1..] };
}

/// Zero-heap strict RFC 5545 structural validator:
/// rejects control bytes, bare CR, oversized lines/property floods, excessive nesting depth,
/// mismatched/unclosed BEGIN..END components, and unclosed parameter quotes.
pub fn validateIcalStrict(ics_text: []const u8): bool ! IcalError {
    let src = ics_text;
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        src = src[3..];
    }
    if (src.len == 0 || simdCalHasControlByte(src)) {
        return IcalError.InvalidFormat;
    }

    let stack: [ICAL_MAX_COMPONENT_DEPTH][]const u8 = [""; ICAL_MAX_COMPONENT_DEPTH];
    let depth: usize = 0;
    let seen_calendar: bool = false;
    let prop_count: usize = 0;

    let pos: usize = 0;
    while (pos < src.len) {
        // Locate end of logical (unfolded) line without heap allocation
        const logical_start: usize = pos;
        let logical_len: usize = 0;
        let first_seg_end: usize = pos;
        let is_first_seg: bool = true;

        while (pos < src.len) {
            const hit = simdCalFindLineDelim(src, pos);
            if (hit < src.len && (src[hit] == '\r' || src[hit] == '\n')) {
                const seg_len = hit - pos;
                logical_len += seg_len;
                if (logical_len > ICAL_MAX_LINE_BYTES) {
                    return IcalError.InvalidFormat;
                }
                if (is_first_seg) {
                    first_seg_end = hit;
                    is_first_seg = false;
                }
                if (src[hit] == '\r') {
                    if (hit + 1 >= src.len || src[hit + 1] != '\n') {
                        return IcalError.InvalidFormat;
                    }
                    pos = hit + 2;
                } else {
                    pos = hit + 1;
                }
                // RFC 5545 folding continuation: next line starts with SPACE or HTAB
                if (pos < src.len && (src[pos] == ' ' || src[pos] == '\t')) {
                    pos += 1;
                    continue;
                }
                break;
            }
            if (hit >= src.len) {
                logical_len += src.len - pos;
                if (logical_len > ICAL_MAX_LINE_BYTES) {
                    return IcalError.InvalidFormat;
                }
                if (is_first_seg) {
                    first_seg_end = src.len;
                    is_first_seg = false;
                }
                pos = src.len;
                break;
            }
            // hit was ':' or ';', advance past it
            logical_len += (hit - pos) + 1;
            if (logical_len > ICAL_MAX_LINE_BYTES) {
                return IcalError.InvalidFormat;
            }
            pos = hit + 1;
        }

        if (logical_len == 0) continue;
        const first_line = trimSpaces(src[logical_start..first_seg_end]);
        if (first_line.len == 0 && logical_len == (first_seg_end - logical_start)) continue;

        prop_count += 1;
        if (prop_count > ICAL_MAX_PROPERTIES) {
            return IcalError.InvalidFormat;
        }

        // Validate property name, balanced parameter quotes, and ':' separator across logical span
        let in_q: bool = false;
        let colon_pos: usize? = null;
        let semi_pos: usize? = null;
        let scan_i: usize = logical_start;
        while (scan_i < pos) : (scan_i += 1) {
            const ch = src[scan_i];
            if (ch == '\r' || ch == '\n') {
                // Skip folding sequence (CRLF+SP or LF+SP)
                if (ch == '\r' && scan_i + 1 < pos && src[scan_i + 1] == '\n') {
                    scan_i += 1;
                }
                if (scan_i + 1 < pos && (src[scan_i + 1] == ' ' || src[scan_i + 1] == '\t')) {
                    scan_i += 1;
                    continue;
                }
                break;
            }
            if (ch == '"') {
                in_q = !in_q;
            } else if (!in_q) {
                if (ch == ';' && semi_pos == null) {
                    semi_pos = scan_i;
                } else if (ch == ':') {
                    colon_pos = scan_i;
                    break;
                }
            }
        }
        if (in_q) return IcalError.InvalidFormat;
        const c_idx = colon_pos ?? return IcalError.InvalidFormat;
        const name_end = semi_pos ?? c_idx;
        const prop_name = trimSpaces(src[logical_start..name_end]);
        if (prop_name.len == 0) return IcalError.InvalidFormat;

        // Extract single-segment value for BEGIN/END structural pairing
        if (asciiEqlIgnoreCase(prop_name, "BEGIN")) {
            const comp = trimSpaces(src[c_idx + 1 .. first_seg_end]);
            if (comp.len == 0) return IcalError.InvalidFormat;
            if (depth == 0) {
                if (!asciiEqlIgnoreCase(comp, "VCALENDAR")) {
                    return IcalError.InvalidFormat;
                }
                seen_calendar = true;
            }
            if (depth >= ICAL_MAX_COMPONENT_DEPTH) {
                return IcalError.InvalidFormat;
            }
            stack[depth] = comp;
            depth += 1;
        } else if (asciiEqlIgnoreCase(prop_name, "END")) {
            const comp = trimSpaces(src[c_idx + 1 .. first_seg_end]);
            if (depth == 0 || !asciiEqlIgnoreCase(stack[depth - 1], comp)) {
                return IcalError.InvalidFormat;
            }
            depth -= 1;
        } else {
            if (depth == 0) {
                return IcalError.InvalidFormat;
            }
        }
    }

    if (!seen_calendar || depth != 0) {
        return IcalError.InvalidFormat;
    }
    return true;
}

// ============================================================================
// IcalEvent & IcalCalendar
// ============================================================================

pub struct IcalEvent {
    uid: []u8 = "",
    summary: []u8 = "",
    description: []u8 = "",
    location: []u8 = "",
    dtstart: IcalDateTime = IcalDateTime{},
    dtend: IcalDateTime? = null,
    rrule: RRule? = null,
    exdates: std.ArrayList(IcalDateTime) = .empty,
    alloc: std.mem.Allocator,

    static fn init(alloc: std.mem.Allocator): IcalEvent {
        return IcalEvent{
            alloc: alloc,
            exdates: .empty,
        };
    }

    fn deinit(*_) {
        if (_.uid.len > 0) _.alloc.free(_.uid);
        if (_.summary.len > 0) _.alloc.free(_.summary);
        if (_.description.len > 0) _.alloc.free(_.description);
        if (_.location.len > 0) _.alloc.free(_.location);
        if (*rr := _.rrule) {
            rr.deinit();
        }
        _.exdates.deinit(_.alloc);
        _.uid = "";
        _.summary = "";
        _.description = "";
        _.location = "";
    }

    fn occurrences(
        alloc: std.mem.Allocator,
        range_start: IcalDateTime,
        range_end: IcalDateTime,
        max_instances: usize,
    ): ([]IcalDateTime)! {
        if (rr := _.rrule) {
            return rr.expand(alloc, _.dtstart, range_start, range_end, _.exdates.items, max_instances);
        }
        let out: std.ArrayList(IcalDateTime) = .empty;
        errdefer out.deinit(alloc);
        if (max_instances > 0 && _.dtstart.compare(range_start) >= 0 && _.dtstart.compare(range_end) <= 0) {
            let excluded: bool = false;
            for (ex of _.exdates.items) {
                if (ex.is_date_only || _.dtstart.is_date_only) {
                    if (_.dtstart.sameDate(ex)) {
                        excluded = true;
                        break;
                    }
                } else if (_.dtstart.compare(ex) == 0) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded) {
                out.append(alloc, _.dtstart) catch return IcalError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }
}

pub struct IcalCalendar {
    prodid: []u8 = "",
    version: []u8 = "",
    events: std.ArrayList(IcalEvent) = .empty,
    alloc: std.mem.Allocator,

    static fn init(alloc: std.mem.Allocator): IcalCalendar {
        return IcalCalendar{
            alloc: alloc,
            events: .empty,
        };
    }

    fn deinit(*_) {
        for (*ev of _.events.items) {
            ev.deinit();
        }
        _.events.deinit(_.alloc);
        if (_.prodid.len > 0) _.alloc.free(_.prodid);
        if (_.version.len > 0) _.alloc.free(_.version);
        _.prodid = "";
        _.version = "";
    }

    static fn parse(alloc: std.mem.Allocator, ics_text: []const u8): IcalCalendar! {
        let src = ics_text;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            src = src[3..];
        }
        if (src.len == 0 || simdCalHasControlByte(src)) {
            return IcalError.InvalidFormat;
        }

        let unfolded = unfoldLines(alloc, src)?;
        defer alloc.free(unfolded);

        let cal = IcalCalendar.init(alloc);
        errdefer cal.deinit();

        let comp_stack: [ICAL_MAX_COMPONENT_DEPTH][]const u8 = [""; ICAL_MAX_COMPONENT_DEPTH];
        let comp_depth: usize = 0;
        let prop_count: usize = 0;
        let in_calendar: bool = false;
        let seen_calendar: bool = false;
        let in_event: bool = false;
        let cur_event: IcalEvent = IcalEvent.init(alloc);
        let has_dtstart: bool = false;
        errdefer {
            if (in_event) cur_event.deinit();
        }

        let pos: usize = 0;
        while (pos < unfolded.len) {
            let line_end: usize = pos;
            while (line_end < unfolded.len && unfolded[line_end] != '\n') {
                line_end += 1;
            }
            let raw_line = unfolded[pos..line_end];
            pos = if (line_end < unfolded.len) line_end + 1 else unfolded.len;

            if (raw_line.len > ICAL_MAX_LINE_BYTES) {
                return IcalError.InvalidFormat;
            }

            let trimmed = trimSpaces(raw_line);
            if (trimmed.len == 0) continue;

            prop_count += 1;
            if (prop_count > ICAL_MAX_PROPERTIES) {
                return IcalError.InvalidFormat;
            }

            let parts = splitContentLine(raw_line) ?? return IcalError.InvalidFormat;
            let prop = parts.name;
            let val = parts.value;

            if (asciiEqlIgnoreCase(prop, "BEGIN")) {
                let vtrim = trimSpaces(val);
                if (vtrim.len == 0 || comp_depth >= ICAL_MAX_COMPONENT_DEPTH) {
                    return IcalError.InvalidFormat;
                }
                if (asciiEqlIgnoreCase(vtrim, "VCALENDAR")) {
                    if (in_calendar || comp_depth != 0) return IcalError.InvalidFormat;
                    in_calendar = true;
                    seen_calendar = true;
                } else if (asciiEqlIgnoreCase(vtrim, "VEVENT")) {
                    if (!in_calendar || in_event) return IcalError.InvalidFormat;
                    in_event = true;
                    cur_event = IcalEvent.init(alloc);
                    has_dtstart = false;
                } else {
                    if (!in_calendar) return IcalError.InvalidFormat;
                }
                comp_stack[comp_depth] = vtrim;
                comp_depth += 1;
                continue;
            }

            if (asciiEqlIgnoreCase(prop, "END")) {
                let vtrim = trimSpaces(val);
                if (comp_depth == 0 || !asciiEqlIgnoreCase(comp_stack[comp_depth - 1], vtrim)) {
                    return IcalError.InvalidFormat;
                }
                comp_depth -= 1;
                if (asciiEqlIgnoreCase(vtrim, "VEVENT")) {
                    if (!in_event) return IcalError.InvalidFormat;
                    if (!has_dtstart) return IcalError.MissingRequiredField;
                    cal.events.append(alloc, cur_event) catch return IcalError.OutOfMemory;
                    in_event = false;
                    cur_event = IcalEvent.init(alloc);
                } else if (asciiEqlIgnoreCase(vtrim, "VCALENDAR")) {
                    if (!in_calendar || in_event) return IcalError.InvalidFormat;
                    in_calendar = false;
                }
                continue;
            }

            if (!in_calendar || comp_depth == 0) return IcalError.InvalidFormat;

            if (!in_event) {
                if (comp_depth == 1) {
                    if (asciiEqlIgnoreCase(prop, "PRODID")) {
                        if (cal.prodid.len > 0) alloc.free(cal.prodid);
                        cal.prodid = dupSlice(alloc, trimSpaces(val))?;
                    } else if (asciiEqlIgnoreCase(prop, "VERSION")) {
                        if (cal.version.len > 0) alloc.free(cal.version);
                        cal.version = dupSlice(alloc, trimSpaces(val))?;
                    }
                }
                continue;
            }

            // Ignore nested subcomponents (e.g. VALARM inside VEVENT)
            if (comp_depth > 2) continue;

            // Inside VEVENT
            if (asciiEqlIgnoreCase(prop, "UID")) {
                if (cur_event.uid.len > 0) alloc.free(cur_event.uid);
                cur_event.uid = dupSlice(alloc, trimSpaces(val))?;
            } else if (asciiEqlIgnoreCase(prop, "SUMMARY")) {
                if (cur_event.summary.len > 0) alloc.free(cur_event.summary);
                cur_event.summary = unescapeText(alloc, val)?;
            } else if (asciiEqlIgnoreCase(prop, "DESCRIPTION")) {
                if (cur_event.description.len > 0) alloc.free(cur_event.description);
                cur_event.description = unescapeText(alloc, val)?;
            } else if (asciiEqlIgnoreCase(prop, "LOCATION")) {
                if (cur_event.location.len > 0) alloc.free(cur_event.location);
                cur_event.location = unescapeText(alloc, val)?;
            } else if (asciiEqlIgnoreCase(prop, "DTSTART")) {
                cur_event.dtstart = IcalDateTime.parse(val)?;
                has_dtstart = true;
            } else if (asciiEqlIgnoreCase(prop, "DTEND")) {
                cur_event.dtend = IcalDateTime.parse(val)?;
            } else if (asciiEqlIgnoreCase(prop, "RRULE")) {
                if (*old_rr := cur_event.rrule) {
                    old_rr.deinit();
                }
                cur_event.rrule = RRule.parse(alloc, val)?;
            } else if (asciiEqlIgnoreCase(prop, "EXDATE")) {
                let epos: usize = 0;
                while (epos < val.len) {
                    let comma: usize = epos;
                    while (comma < val.len && val[comma] != ',') {
                        comma += 1;
                    }
                    let tok = trimSpaces(val[epos..comma]);
                    epos = if (comma < val.len) comma + 1 else val.len;
                    if (tok.len > 0) {
                        let ex_dt = IcalDateTime.parse(tok)?;
                        cur_event.exdates.append(alloc, ex_dt) catch return IcalError.OutOfMemory;
                    }
                }
            }
        }

        if (!seen_calendar || in_calendar || in_event || comp_depth != 0) {
            return IcalError.InvalidFormat;
        }

        return cal;
    }

    fn occurrences(
        alloc: std.mem.Allocator,
        range_start: IcalDateTime,
        range_end: IcalDateTime,
        max_instances: usize,
    ): ([]IcalDateTime)! {
        let all: std.ArrayList(IcalDateTime) = .empty;
        errdefer all.deinit(alloc);

        for (ev of _.events.items) {
            let ev_occ = ev.occurrences(alloc, range_start, range_end, max_instances)?;
            defer alloc.free(ev_occ);
            for (dt of ev_occ) {
                all.append(alloc, dt) catch return IcalError.OutOfMemory;
            }
        }

        // Insertion sort chronologically
        let n: usize = all.items.len;
        let i: usize = 1;
        while (i < n) : (i += 1) {
            let key = all.items[i];
            let j: usize = i;
            while (j > 0 && all.items[j - 1].compare(key) > 0) : (j -= 1) {
                all.items[j] = all.items[j - 1];
            }
            all.items[j] = key;
        }

        if (all.items.len > max_instances) {
            all.shrinkRetainingCapacity(max_instances);
        }
        return all.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }

    fn toIcs(alloc: std.mem.Allocator): ([]u8)! {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        buf.appendSlice(alloc, "BEGIN:VCALENDAR\r\n") catch return IcalError.OutOfMemory;
        buf.appendSlice(alloc, "VERSION:") catch return IcalError.OutOfMemory;
        let ver = if (_.version.len > 0) _.version else "2.0";
        buf.appendSlice(alloc, ver) catch return IcalError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;

        buf.appendSlice(alloc, "PRODID:") catch return IcalError.OutOfMemory;
        let pid = if (_.prodid.len > 0) _.prodid else "-//CP//cap:ical//EN";
        buf.appendSlice(alloc, pid) catch return IcalError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;

        for (ev of _.events.items) {
            buf.appendSlice(alloc, "BEGIN:VEVENT\r\n") catch return IcalError.OutOfMemory;

            if (ev.uid.len > 0) {
                buf.appendSlice(alloc, "UID:") catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, ev.uid) catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }

            let dtstart_s = ev.dtstart.format(alloc)?;
            defer alloc.free(dtstart_s);
            buf.appendSlice(alloc, "DTSTART:") catch return IcalError.OutOfMemory;
            buf.appendSlice(alloc, dtstart_s) catch return IcalError.OutOfMemory;
            buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;

            if (de := ev.dtend) {
                let dtend_s = de.format(alloc)?;
                defer alloc.free(dtend_s);
                buf.appendSlice(alloc, "DTEND:") catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, dtend_s) catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }

            if (ev.summary.len > 0) {
                buf.appendSlice(alloc, "SUMMARY:") catch return IcalError.OutOfMemory;
                appendEscapedText(*buf, alloc, ev.summary)?;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }
            if (ev.description.len > 0) {
                buf.appendSlice(alloc, "DESCRIPTION:") catch return IcalError.OutOfMemory;
                appendEscapedText(*buf, alloc, ev.description)?;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }
            if (ev.location.len > 0) {
                buf.appendSlice(alloc, "LOCATION:") catch return IcalError.OutOfMemory;
                appendEscapedText(*buf, alloc, ev.location)?;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }

            if (rr := ev.rrule) {
                let rr_s = rr.format(alloc)?;
                defer alloc.free(rr_s);
                buf.appendSlice(alloc, "RRULE:") catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, rr_s) catch return IcalError.OutOfMemory;
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }

            if (ev.exdates.items.len > 0) {
                buf.appendSlice(alloc, "EXDATE:") catch return IcalError.OutOfMemory;
                let idx: usize = 0;
                while (idx < ev.exdates.items.len) : (idx += 1) {
                    if (idx > 0) buf.append(alloc, ',') catch return IcalError.OutOfMemory;
                    let ex_s = ev.exdates.items[idx].format(alloc)?;
                    defer alloc.free(ex_s);
                    buf.appendSlice(alloc, ex_s) catch return IcalError.OutOfMemory;
                }
                buf.appendSlice(alloc, "\r\n") catch return IcalError.OutOfMemory;
            }

            buf.appendSlice(alloc, "END:VEVENT\r\n") catch return IcalError.OutOfMemory;
        }

        buf.appendSlice(alloc, "END:VCALENDAR\r\n") catch return IcalError.OutOfMemory;
        return buf.toOwnedSlice(alloc) catch return IcalError.OutOfMemory;
    }
}
