// Source: cp/lib/tz/tz.cp (Pure CP Timezone, POSIX TZ & DST Rule Engine)
// Zero C dependencies; implements Howard Hinnant O(1) civil calendar, POSIX TZ parser, IANA lookup & RFC 3339 formatting.

const std = @import("std");

pub err TzError {
    UnknownTimezone,
    InvalidTzString,
    InvalidOffset,
    InvalidDateTime,
    OutOfMemory,
}

pub struct DstRule {
    month: u8;
    week: u8;
    weekday: u8;
    time_sec: i32;

    fn resolveDays(year: i32): i64 {
        let dim = daysInMonth(year, _.month);
        let first_day = daysFromCivil(year, _.month, 1);
        let first_wday = weekdayFromDays(first_day);
        let target_wday: i64 = i64(_.weekday);
        let fw: i64 = i64(first_wday);
        let delta: i64 = @mod(target_wday - fw + 7, 7);

        let day_of_month: i64 = 1;
        if (_.week == 5) {
            day_of_month = 1 + delta + 28;
            while (day_of_month > dim) {
                day_of_month -= 7;
            }
        } else {
            let wk_idx: i64 = i64(_.week);
            day_of_month = 1 + delta + (wk_idx - 1) * 7;
            if (day_of_month > dim) {
                day_of_month -= 7;
            }
        }
        return daysFromCivil(year, _.month, u8(day_of_month));
    }
}

pub struct LocalDateTime {
    year: i32;
    month: u8;
    day: u8;
    hour: u8;
    minute: u8;
    second: u8;
    offset_sec: i32;
    is_dst: bool;
    abbr: []const u8;

    fn dayOfWeek(): u8 {
        let z = daysFromCivil(_.year, _.month, _.day);
        return weekdayFromDays(z);
    }

    fn formatRfc3339(alloc: std.mem.Allocator): ([]u8) ! TzError {
        return formatLocalRfc3339(alloc, _);
    }
}

pub struct Timezone {
    name: []const u8;
    std_abbr: []const u8;
    dst_abbr: []const u8;
    std_offset_sec: i32;
    dst_offset_sec: i32;
    has_dst: bool;
    dst_start: DstRule;
    dst_end: DstRule;

    static fn utc(): Timezone {
        return Timezone{
            name: "UTC",
            std_abbr: "UTC",
            dst_abbr: "",
            std_offset_sec: 0,
            dst_offset_sec: 0,
            has_dst: false,
            dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
        };
    }

    static fn fixedOffset(tz_name: []const u8, offset_sec: i32): Timezone {
        return Timezone{
            name: tz_name,
            std_abbr: tz_name,
            dst_abbr: "",
            std_offset_sec: offset_sec,
            dst_offset_sec: offset_sec,
            has_dst: false,
            dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
        };
    }

    static fn parseOffset(s: []const u8): i32 ! TzError {
        let trimmed = trimSpaces(s);
        if (trimmed.len == 0) return TzError.InvalidOffset;

        if (std.mem.eql(u8, trimmed, "Z") || std.mem.eql(u8, trimmed, "z") ||
            std.mem.eql(u8, trimmed, "UTC") || std.mem.eql(u8, trimmed, "GMT"))
        {
            return 0;
        }

        let rest = trimmed;
        if (rest.len >= 3 && (std.mem.eql(u8, rest[0..3], "UTC") || std.mem.eql(u8, rest[0..3], "GMT"))) {
            rest = rest[3..];
            if (rest.len == 0) return 0;
        }

        let sign: i32 = 1;
        if (rest[0] == '+') {
            sign = 1;
            rest = rest[1..];
        } else if (rest[0] == '-') {
            sign = -1;
            rest = rest[1..];
        } else {
            return TzError.InvalidOffset;
        }

        if (rest.len == 0) return TzError.InvalidOffset;

        let hours: i32 = 0;
        let minutes: i32 = 0;
        let seconds: i32 = 0;

        if (colon_idx := std.mem.indexOfScalar(u8, rest, ':')) {
            let h_str = rest[0..colon_idx];
            if (h_str.len == 0 || h_str.len > 2) return TzError.InvalidOffset;
            hours = parseUnsignedDigits(h_str) catch return TzError.InvalidOffset;
            let after_h = rest[colon_idx + 1..];
            if (colon2 := std.mem.indexOfScalar(u8, after_h, ':')) {
                let m_str = after_h[0..colon2];
                let s_str = after_h[colon2 + 1..];
                if (m_str.len != 2 || s_str.len != 2) return TzError.InvalidOffset;
                minutes = parseUnsignedDigits(m_str) catch return TzError.InvalidOffset;
                seconds = parseUnsignedDigits(s_str) catch return TzError.InvalidOffset;
            } else {
                if (after_h.len != 2) return TzError.InvalidOffset;
                minutes = parseUnsignedDigits(after_h) catch return TzError.InvalidOffset;
            }
        } else {
            if (rest.len == 1 || rest.len == 2) {
                hours = parseUnsignedDigits(rest) catch return TzError.InvalidOffset;
            } else if (rest.len == 4) {
                hours = parseUnsignedDigits(rest[0..2]) catch return TzError.InvalidOffset;
                minutes = parseUnsignedDigits(rest[2..4]) catch return TzError.InvalidOffset;
            } else {
                return TzError.InvalidOffset;
            }
        }

        if (hours > 23 || minutes > 59 || seconds > 59) {
            return TzError.InvalidOffset;
        }
        return sign * (hours * 3600 + minutes * 60 + seconds);
    }

    static fn parsePosix(posix_str: []const u8): Timezone ! TzError {
        let s = trimSpaces(posix_str);
        if (s.len == 0) return TzError.InvalidTzString;

        let idx: usize = 0;
        let std_abbr = parseTzAbbr(s, *idx)?;

        // POSIX TZ offset sign is inverted relative to ISO 8601:
        // "EST5" means local + 5h = UTC, so UTC offset is -18000s.
        let std_posix_off = parsePosixOffset(s, *idx)?;
        let std_offset_sec: i32 = -std_posix_off;

        if (idx >= s.len) {
            return Timezone{
                name: s,
                std_abbr: std_abbr,
                dst_abbr: "",
                std_offset_sec: std_offset_sec,
                dst_offset_sec: std_offset_sec,
                has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }

        let dst_abbr = parseTzAbbr(s, *idx)?;
        let dst_offset_sec: i32 = std_offset_sec + 3600;
        if (idx < s.len && s[idx] != ',') {
            let dst_posix_off = parsePosixOffset(s, *idx)?;
            dst_offset_sec = -dst_posix_off;
        }

        if (idx >= s.len || s[idx] != ',') {
            return TzError.InvalidTzString;
        }
        idx += 1;

        let start_rule = parseDstRule(s, *idx)?;
        if (idx >= s.len || s[idx] != ',') {
            return TzError.InvalidTzString;
        }
        idx += 1;

        let end_rule = parseDstRule(s, *idx)?;
        if (idx != s.len) {
            return TzError.InvalidTzString;
        }

        return Timezone{
            name: s,
            std_abbr: std_abbr,
            dst_abbr: dst_abbr,
            std_offset_sec: std_offset_sec,
            dst_offset_sec: dst_offset_sec,
            has_dst: true,
            dst_start: start_rule,
            dst_end: end_rule,
        };
    }

    static fn lookup(iana_name: []const u8): Timezone ! TzError {
        if (std.mem.eql(u8, iana_name, "UTC") || std.mem.eql(u8, iana_name, "Etc/UTC") || std.mem.eql(u8, iana_name, "GMT")) {
            return Timezone.utc();
        }
        if (std.mem.eql(u8, iana_name, "Asia/Shanghai") || std.mem.eql(u8, iana_name, "Asia/Chongqing") ||
            std.mem.eql(u8, iana_name, "Asia/Harbin") || std.mem.eql(u8, iana_name, "Asia/Urumqi"))
        {
            return Timezone{
                name: iana_name, std_abbr: "CST", dst_abbr: "",
                std_offset_sec: 28800, dst_offset_sec: 28800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Hong_Kong")) {
            return Timezone{
                name: iana_name, std_abbr: "HKT", dst_abbr: "",
                std_offset_sec: 28800, dst_offset_sec: 28800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Taipei")) {
            return Timezone{
                name: iana_name, std_abbr: "CST", dst_abbr: "",
                std_offset_sec: 28800, dst_offset_sec: 28800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Singapore")) {
            return Timezone{
                name: iana_name, std_abbr: "SGT", dst_abbr: "",
                std_offset_sec: 28800, dst_offset_sec: 28800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Tokyo")) {
            return Timezone{
                name: iana_name, std_abbr: "JST", dst_abbr: "",
                std_offset_sec: 32400, dst_offset_sec: 32400, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Seoul")) {
            return Timezone{
                name: iana_name, std_abbr: "KST", dst_abbr: "",
                std_offset_sec: 32400, dst_offset_sec: 32400, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Kolkata")) {
            return Timezone{
                name: iana_name, std_abbr: "IST", dst_abbr: "",
                std_offset_sec: 19800, dst_offset_sec: 19800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Dubai")) {
            return Timezone{
                name: iana_name, std_abbr: "GST", dst_abbr: "",
                std_offset_sec: 14400, dst_offset_sec: 14400, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Asia/Bangkok") || std.mem.eql(u8, iana_name, "Asia/Jakarta")) {
            return Timezone{
                name: iana_name, std_abbr: "ICT", dst_abbr: "",
                std_offset_sec: 25200, dst_offset_sec: 25200, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "America/New_York") || std.mem.eql(u8, iana_name, "America/Toronto")) {
            let tz = Timezone.parsePosix("EST5EDT,M3.2.0,M11.1.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Chicago")) {
            let tz = Timezone.parsePosix("CST6CDT,M3.2.0,M11.1.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Denver")) {
            let tz = Timezone.parsePosix("MST7MDT,M3.2.0,M11.1.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Phoenix")) {
            return Timezone{
                name: iana_name, std_abbr: "MST", dst_abbr: "",
                std_offset_sec: -25200, dst_offset_sec: -25200, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Los_Angeles") || std.mem.eql(u8, iana_name, "America/Vancouver")) {
            let tz = Timezone.parsePosix("PST8PDT,M3.2.0,M11.1.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Anchorage")) {
            let tz = Timezone.parsePosix("AKST9AKDT,M3.2.0,M11.1.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "Pacific/Honolulu")) {
            return Timezone{
                name: iana_name, std_abbr: "HST", dst_abbr: "",
                std_offset_sec: -36000, dst_offset_sec: -36000, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "America/Sao_Paulo")) {
            return Timezone{
                name: iana_name, std_abbr: "BRT", dst_abbr: "",
                std_offset_sec: -10800, dst_offset_sec: -10800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Europe/London")) {
            let tz = Timezone.parsePosix("GMT0BST,M3.5.0/1,M10.5.0")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "Europe/Berlin") || std.mem.eql(u8, iana_name, "Europe/Paris") ||
            std.mem.eql(u8, iana_name, "Europe/Rome") || std.mem.eql(u8, iana_name, "Europe/Madrid") ||
            std.mem.eql(u8, iana_name, "Europe/Amsterdam") || std.mem.eql(u8, iana_name, "Europe/Zurich"))
        {
            let tz = Timezone.parsePosix("CET-1CEST,M3.5.0,M10.5.0/3")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "Europe/Moscow")) {
            return Timezone{
                name: iana_name, std_abbr: "MSK", dst_abbr: "",
                std_offset_sec: 10800, dst_offset_sec: 10800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Australia/Sydney") || std.mem.eql(u8, iana_name, "Australia/Melbourne")) {
            let tz = Timezone.parsePosix("AEST-10AEDT,M10.1.0,M4.1.0/3")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        if (std.mem.eql(u8, iana_name, "Australia/Perth")) {
            return Timezone{
                name: iana_name, std_abbr: "AWST", dst_abbr: "",
                std_offset_sec: 28800, dst_offset_sec: 28800, has_dst: false,
                dst_start: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
                dst_end: DstRule{ month: 1, week: 1, weekday: 0, time_sec: 0 },
            };
        }
        if (std.mem.eql(u8, iana_name, "Pacific/Auckland")) {
            let tz = Timezone.parsePosix("NZST-12NZDT,M9.5.0,M4.1.0/3")?;
            return Timezone{
                name: iana_name, std_abbr: tz.std_abbr, dst_abbr: tz.dst_abbr,
                std_offset_sec: tz.std_offset_sec, dst_offset_sec: tz.dst_offset_sec,
                has_dst: tz.has_dst, dst_start: tz.dst_start, dst_end: tz.dst_end,
            };
        }
        return TzError.UnknownTimezone;
    }

    fn isDstAt(unix_sec: i64): bool {
        if (!_.has_dst) return false;

        let approx_days = @divFloor(unix_sec + _.std_offset_sec, 86400);
        let civil = civilFromDays(approx_days);
        let year = civil.year;

        let start_days = _.dst_start.resolveDays(year);
        let end_days = _.dst_end.resolveDays(year);
        let start_utc: i64 = start_days * 86400 + _.dst_start.time_sec - _.std_offset_sec;
        let end_utc: i64 = end_days * 86400 + _.dst_end.time_sec - _.dst_offset_sec;

        if (start_utc < end_utc) {
            return unix_sec >= start_utc && unix_sec < end_utc;
        }
        // Southern hemisphere: DST spans across the calendar year boundary
        return unix_sec < end_utc || unix_sec >= start_utc;
    }

    fn offsetAt(unix_sec: i64): i32 {
        if (_.isDstAt(unix_sec)) {
            return _.dst_offset_sec;
        }
        return _.std_offset_sec;
    }

    fn toLocal(unix_sec: i64): LocalDateTime ! TzError {
        let is_dst = _.isDstAt(unix_sec);
        let off: i32 = if (is_dst) _.dst_offset_sec else _.std_offset_sec;
        let abbr: []const u8 = if (is_dst) _.dst_abbr else _.std_abbr;

        let local_sec: i64 = unix_sec + off;
        let days = @divFloor(local_sec, 86400);
        let sec_of_day = @mod(local_sec, 86400);

        let civil = civilFromDays(days);
        let hour = u8(@divTrunc(sec_of_day, 3600));
        let rem_sec = @mod(sec_of_day, 3600);
        let minute = u8(@divTrunc(rem_sec, 60));
        let second = u8(@mod(rem_sec, 60));

        return LocalDateTime{
            year: civil.year,
            month: civil.month,
            day: civil.day,
            hour: hour,
            minute: minute,
            second: second,
            offset_sec: off,
            is_dst: is_dst,
            abbr: abbr,
        };
    }

    fn toUnix(year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8): i64 ! TzError {
        if (!isValidDateTime(year, month, day, hour, minute, second)) {
            return TzError.InvalidDateTime;
        }

        let days = daysFromCivil(year, month, day);
        let local_sec: i64 = days * 86400 + i64(hour) * 3600 + i64(minute) * 60 + i64(second);

        if (!_.has_dst) {
            return local_sec - _.std_offset_sec;
        }

        let cand_dst: i64 = local_sec - _.dst_offset_sec;
        let cand_std: i64 = local_sec - _.std_offset_sec;
        if (_.isDstAt(cand_dst)) {
            return cand_dst;
        }
        return cand_std;
    }

    fn formatRfc3339(alloc: std.mem.Allocator, unix_sec: i64): ([]u8) ! TzError {
        let dt = _.toLocal(unix_sec)?;
        return formatLocalRfc3339(alloc, dt);
    }
}

// --- Howard Hinnant O(1) Civil Calendar Helpers ---

pub fn isLeapYear(year: i32): bool {
    if (@mod(year, 4) != 0) return false;
    if (@mod(year, 100) != 0) return true;
    return @mod(year, 400) == 0;
}

pub fn daysInMonth(year: i32, month: u8): u8 {
    return switch (month) {
        1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
        4 | 6 | 9 | 11 => 30,
        2 => if (isLeapYear(year)) 29 else 28,
        else => 0,
    };
}

pub fn isValidDateTime(year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8): bool {
    if (month < 1 || month > 12) return false;
    let dim = daysInMonth(year, month);
    if (day < 1 || day > dim) return false;
    if (hour > 23 || minute > 59 || second > 59) return false;
    return true;
}

pub fn daysFromCivil(y: i32, m: u8, d: u8): i64 {
    let yy: i64 = i64(y);
    let mm: i64 = i64(m);
    let dd: i64 = i64(d);
    if (mm <= 2) {
        yy -= 1;
    }
    let era: i64 = if (yy >= 0) @divTrunc(yy, 400) else @divTrunc(yy - 399, 400);
    let yoe: i64 = yy - era * 400;
    let m_adj: i64 = if (mm > 2) mm - 3 else mm + 9;
    let doy: i64 = @divTrunc(153 * m_adj + 2, 5) + dd - 1;
    let doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

const CivilDate = struct {
    year: i32;
    month: u8;
    day: u8;
};

fn civilFromDays(z: i64): CivilDate {
    let zz: i64 = z + 719468;
    let era: i64 = if (zz >= 0) @divTrunc(zz, 146097) else @divTrunc(zz - 146096, 146097);
    let doe: i64 = zz - era * 146097;
    let yoe: i64 = @divTrunc(doe - @divTrunc(doe, 1460) + @divTrunc(doe, 36524) - @divTrunc(doe, 146096), 365);
    let y: i64 = yoe + era * 400;
    let doy: i64 = doe - (365 * yoe + @divTrunc(yoe, 4) - @divTrunc(yoe, 100));
    let mp: i64 = @divTrunc(5 * doy + 2, 153);
    let d: i64 = doy - @divTrunc(153 * mp + 2, 5) + 1;
    let m: i64 = if (mp < 10) mp + 3 else mp - 9;
    if (m <= 2) {
        y += 1;
    }
    return CivilDate{ year: i32(y), month: u8(m), day: u8(d) };
}

pub fn weekdayFromDays(z: i64): u8 {
    // 1970-01-01 (day 0) was Thursday (4)
    return u8(@mod(z + 4, 7));
}

// --- Internal Parsing & Formatting Helpers ---

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[start..end];
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn parseUnsignedDigits(s: []const u8): i32 ! TzError {
    if (s.len == 0) return TzError.InvalidOffset;
    let val: i32 = 0;
    for (ch of s) {
        if (!isDigit(ch)) return TzError.InvalidOffset;
        val = val * 10 + i32(ch - '0');
    }
    return val;
}

fn parseTzAbbr(s: []const u8, idx: *usize): ([]const u8) ! TzError {
    if (idx.* >= s.len) return TzError.InvalidTzString;
    if (s[idx.*] == '<') {
        idx.* += 1;
        let start = idx.*;
        while (idx.* < s.len && s[idx.*] != '>') {
            idx.* += 1;
        }
        if (idx.* >= s.len || idx.* == start) return TzError.InvalidTzString;
        let abbr = s[start..idx.*];
        idx.* += 1;
        return abbr;
    }
    let start = idx.*;
    while (idx.* < s.len && isAlpha(s[idx.*])) {
        idx.* += 1;
    }
    if (idx.* - start < 3) return TzError.InvalidTzString;
    return s[start..idx.*];
}

fn parsePosixOffset(s: []const u8, idx: *usize): i32 ! TzError {
    if (idx.* >= s.len) return TzError.InvalidTzString;
    let sign: i32 = 1;
    if (s[idx.*] == '+') {
        sign = 1;
        idx.* += 1;
    } else if (s[idx.*] == '-') {
        sign = -1;
        idx.* += 1;
    }

    let h = parseIntSegment(s, idx, 1, 2) catch return TzError.InvalidTzString;
    if (h > 24) return TzError.InvalidTzString;

    let m: i32 = 0;
    let sec: i32 = 0;
    if (idx.* < s.len && s[idx.*] == ':') {
        idx.* += 1;
        m = parseIntSegment(s, idx, 1, 2) catch return TzError.InvalidTzString;
        if (m > 59) return TzError.InvalidTzString;
        if (idx.* < s.len && s[idx.*] == ':') {
            idx.* += 1;
            sec = parseIntSegment(s, idx, 1, 2) catch return TzError.InvalidTzString;
            if (sec > 59) return TzError.InvalidTzString;
        }
    }
    return sign * (h * 3600 + m * 60 + sec);
}

fn parseIntSegment(s: []const u8, idx: *usize, min_digits: usize, max_digits: usize): i32 ! TzError {
    let start = idx.*;
    let val: i32 = 0;
    while (idx.* < s.len && isDigit(s[idx.*]) && (idx.* - start) < max_digits) {
        val = val * 10 + i32(s[idx.*] - '0');
        idx.* += 1;
    }
    if (idx.* - start < min_digits) return TzError.InvalidTzString;
    return val;
}

fn parseDstRule(s: []const u8, idx: *usize): DstRule ! TzError {
    if (idx.* >= s.len || s[idx.*] != 'M') return TzError.InvalidTzString;
    idx.* += 1;

    let month_i = parseIntSegment(s, idx, 1, 2)?;
    if (month_i < 1 || month_i > 12) return TzError.InvalidTzString;
    if (idx.* >= s.len || s[idx.*] != '.') return TzError.InvalidTzString;
    idx.* += 1;

    let week_i = parseIntSegment(s, idx, 1, 1)?;
    if (week_i < 1 || week_i > 5) return TzError.InvalidTzString;
    if (idx.* >= s.len || s[idx.*] != '.') return TzError.InvalidTzString;
    idx.* += 1;

    let wday_i = parseIntSegment(s, idx, 1, 1)?;
    if (wday_i < 0 || wday_i > 6) return TzError.InvalidTzString;

    let time_sec: i32 = 7200; // Default 02:00:00
    if (idx.* < s.len && s[idx.*] == '/') {
        idx.* += 1;
        let t_sign: i32 = 1;
        if (idx.* < s.len && s[idx.*] == '-') {
            t_sign = -1;
            idx.* += 1;
        } else if (idx.* < s.len && s[idx.*] == '+') {
            idx.* += 1;
        }
        let th = parseIntSegment(s, idx, 1, 3)?;
        if (th > 167) return TzError.InvalidTzString;
        let tm: i32 = 0;
        let ts: i32 = 0;
        if (idx.* < s.len && s[idx.*] == ':') {
            idx.* += 1;
            tm = parseIntSegment(s, idx, 1, 2)?;
            if (tm > 59) return TzError.InvalidTzString;
            if (idx.* < s.len && s[idx.*] == ':') {
                idx.* += 1;
                ts = parseIntSegment(s, idx, 1, 2)?;
                if (ts > 59) return TzError.InvalidTzString;
            }
        }
        time_sec = t_sign * (th * 3600 + tm * 60 + ts);
    }

    return DstRule{
        month: u8(month_i),
        week: u8(week_i),
        weekday: u8(wday_i),
        time_sec: time_sec,
    };
}

fn writeTwoDigits(buf: []u8, pos: usize, val: i32) {
    let v = if (val < 0) 0 else val;
    buf[pos] = u8('0' + @mod(@divTrunc(v, 10), 10));
    buf[pos + 1] = u8('0' + @mod(v, 10));
}

fn formatLocalRfc3339(alloc: std.mem.Allocator, dt: LocalDateTime): ([]u8) ! TzError {
    let len: usize = if (dt.offset_sec == 0) 20 else 25;
    let buf = alloc.alloc(u8, len) catch return TzError.OutOfMemory;

    let y = if (dt.year < 0) 0 else dt.year;
    buf[0] = u8('0' + @mod(@divTrunc(y, 1000), 10));
    buf[1] = u8('0' + @mod(@divTrunc(y, 100), 10));
    buf[2] = u8('0' + @mod(@divTrunc(y, 10), 10));
    buf[3] = u8('0' + @mod(y, 10));
    buf[4] = '-';
    writeTwoDigits(buf, 5, i32(dt.month));
    buf[7] = '-';
    writeTwoDigits(buf, 8, i32(dt.day));
    buf[10] = 'T';
    writeTwoDigits(buf, 11, i32(dt.hour));
    buf[13] = ':';
    writeTwoDigits(buf, 14, i32(dt.minute));
    buf[16] = ':';
    writeTwoDigits(buf, 17, i32(dt.second));

    if (dt.offset_sec == 0) {
        buf[19] = 'Z';
        return buf;
    }

    buf[19] = if (dt.offset_sec >= 0) '+' else '-';
    let abs_off: i32 = if (dt.offset_sec >= 0) dt.offset_sec else (0 - dt.offset_sec);
    let off_h: i32 = @divTrunc(abs_off, 3600);
    let off_m: i32 = @divTrunc(@mod(abs_off, 3600), 60);
    writeTwoDigits(buf, 20, off_h);
    buf[22] = ':';
    writeTwoDigits(buf, 23, off_m);
    return buf;
}
