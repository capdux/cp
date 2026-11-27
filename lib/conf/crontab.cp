// Source: cp/lib/crontab/crontab.cp (Pure CP 5-Field & Macro Cron Parser and Next-Trigger Engine)
const std = @import("std");

pub const CronError = error{
    InvalidFieldCount,
    InvalidNumber,
    OutOfRange,
    InvalidRange,
    InvalidStep,
    EmptyField,
};

pub const DateTime = struct {
    year: i32,
    month: u8, // 1..12
    day: u8, // 1..31
    hour: u8, // 0..23
    minute: u8, // 0..59

    pub static fn isLeapYear(year: i32): bool {
        if (@mod(year, 400) == 0) { return true; }
        if (@mod(year, 100) == 0) { return false; }
        return @mod(year, 4) == 0;
    }

    pub static fn daysInMonth(year: i32, month: u8): u8 {
        switch (month) {
            1, 3, 5, 7, 8, 10, 12 => return 31,
            4, 6, 9, 11 => return 30,
            2 => return if (DateTime.isLeapYear(year)) 29 else 28,
            else => return 30,
        }
    }

    /// Tomohiko Sakamoto's algorithm: 0 = Sunday .. 6 = Saturday
    pub fn weekday(): u8 {
        const t: [12]i32 = [ 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 ];
        let y: i32 = _.year;
        if (_.month < 3) { y -= 1; }
        const m_idx: usize = @intCast(_.month - 1);
        const w = @mod(y + @divFloor(y, 4) - @divFloor(y, 100) + @divFloor(y, 400) + t[m_idx] + @as(i32, _.day), 7);
        return @intCast(w);
    }

    pub fn addOneMinute(): DateTime {
        let dt = DateTime{
            .year = _.year,
            .month = _.month,
            .day = _.day,
            .hour = _.hour,
            .minute = _.minute,
        };
        dt.minute += 1;
        if (dt.minute < 60) { return dt; }
        dt.minute = 0;
        dt.hour += 1;
        if (dt.hour < 24) { return dt; }
        dt.hour = 0;
        dt.day += 1;
        const dim = DateTime.daysInMonth(dt.year, dt.month);
        if (dt.day <= dim) { return dt; }
        dt.day = 1;
        dt.month += 1;
        if (dt.month <= 12) { return dt; }
        dt.month = 1;
        dt.year += 1;
        return dt;
    }

    pub static fn fromUnixSeconds(unix_sec: i64): DateTime {
        const total_days = @divFloor(unix_sec, 86400);
        const rem_sec: i64 = @mod(unix_sec, 86400);
        const hour: u8 = @intCast(@divFloor(rem_sec, 3600));
        const minute: u8 = @intCast(@divFloor(@mod(rem_sec, 3600), 60));

        // Howard Hinnant's civil-from-days algorithm (epoch 1970-01-01)
        const z = total_days + 719468;
        const era = @divFloor(if (z >= 0) z else z - 146096, 146097);
        const doe: i64 = z - era * 146097;
        const yoe = @divFloor(doe - @divFloor(doe, 1460) + @divFloor(doe, 36524) - @divFloor(doe, 146096), 365);
        const y_base = yoe + era * 400;
        const doy = doe - (365 * yoe + @divFloor(yoe, 4) - @divFloor(yoe, 100));
        const mp = @divFloor(5 * doy + 2, 153);
        const d: u8 = @intCast(doy - @divFloor(153 * mp + 2, 5) + 1);
        const m: u8 = @intCast(if (mp < 10) mp + 3 else mp - 9);
        const y: i32 = @intCast(y_base + (if (m <= 2) @as(i64, 1) else @as(i64, 0)));
        return DateTime{ .year = y, .month = m, .day = d, .hour = hour, .minute = minute };
    }
};

pub type OptDateTime = DateTime?;

fn parseUint(s: []const u8): CronError!u8 {
    if (s.len == 0) { return CronError.InvalidNumber; }
    let val: u16 = 0;
    for (c of s) {
        if (c < '0' or c > '9') { return CronError.InvalidNumber; }
        val = val * 10 + @as(u16, c - '0');
        if (val > 255) { return CronError.OutOfRange; }
    }
    return @intCast(val);
}

fn parseNamedMonth(s: []const u8): u8? {
    if (std.ascii.eqlIgnoreCase(s, "jan")) { return 1; }
    if (std.ascii.eqlIgnoreCase(s, "feb")) { return 2; }
    if (std.ascii.eqlIgnoreCase(s, "mar")) { return 3; }
    if (std.ascii.eqlIgnoreCase(s, "apr")) { return 4; }
    if (std.ascii.eqlIgnoreCase(s, "may")) { return 5; }
    if (std.ascii.eqlIgnoreCase(s, "jun")) { return 6; }
    if (std.ascii.eqlIgnoreCase(s, "jul")) { return 7; }
    if (std.ascii.eqlIgnoreCase(s, "aug")) { return 8; }
    if (std.ascii.eqlIgnoreCase(s, "sep")) { return 9; }
    if (std.ascii.eqlIgnoreCase(s, "oct")) { return 10; }
    if (std.ascii.eqlIgnoreCase(s, "nov")) { return 11; }
    if (std.ascii.eqlIgnoreCase(s, "dec")) { return 12; }
    return null;
}

fn parseNamedDow(s: []const u8): u8? {
    if (std.ascii.eqlIgnoreCase(s, "sun")) { return 0; }
    if (std.ascii.eqlIgnoreCase(s, "mon")) { return 1; }
    if (std.ascii.eqlIgnoreCase(s, "tue")) { return 2; }
    if (std.ascii.eqlIgnoreCase(s, "wed")) { return 3; }
    if (std.ascii.eqlIgnoreCase(s, "thu")) { return 4; }
    if (std.ascii.eqlIgnoreCase(s, "fri")) { return 5; }
    if (std.ascii.eqlIgnoreCase(s, "sat")) { return 6; }
    return null;
}

fn parseFieldAtom(s: []const u8, min_val: u8, max_val: u8, is_month: bool, is_dow: bool): CronError!u8 {
    if (is_month) {
        if (m := parseNamedMonth(s)) { return m; }
    }
    if (is_dow) {
        if (d := parseNamedDow(s)) { return d; }
    }
    let v = try parseUint(s);
    if (is_dow and v == 7) { v = 0; }
    if (v < min_val or v > max_val) { return CronError.OutOfRange; }
    return v;
}

fn parseBitmask(field: []const u8, min_val: u8, max_val: u8, is_month: bool, is_dow: bool): CronError!u64 {
    if (field.len == 0) { return CronError.EmptyField; }
    let mask: u64 = 0;
    let it = std.mem.splitScalar(u8, field, ',');
    while (part := it.next()) {
        if (part.len == 0) { return CronError.EmptyField; }
        let step: u8 = 1;
        let range_part = part;
        if (slash_pos := std.mem.indexOfScalar(u8, part, '/')) {
            range_part = part[0..slash_pos];
            const step_str = part[slash_pos + 1 ..];
            step = try parseUint(step_str);
            if (step == 0) { return CronError.InvalidStep; }
        }

        let start: u8 = min_val;
        let end: u8 = max_val;
        if (std.mem.eql(u8, range_part, "*")) {
            start = min_val;
            end = max_val;
        } else if (dash_pos := std.mem.indexOfScalar(u8, range_part, '-')) {
            start = try parseFieldAtom(range_part[0..dash_pos], min_val, max_val, is_month, is_dow);
            end = try parseFieldAtom(range_part[dash_pos + 1 ..], min_val, max_val, is_month, is_dow);
            if (start > end) { return CronError.InvalidRange; }
        } else {
            start = try parseFieldAtom(range_part, min_val, max_val, is_month, is_dow);
            end = if (std.mem.indexOfScalar(u8, part, '/') != null) max_val else start;
        }

        let v: u8 = start;
        while (v <= end) {
            const bit_idx: u6 = @intCast(v);
            mask |= (@as(u64, 1) << bit_idx);
            if (@as(u16, v) + @as(u16, step) > @as(u16, end)) { break; }
            v += step;
        }
    }
    return mask;
}

pub const CronExpr = struct {
    minutes: u64,
    hours: u64,
    days_of_month: u64,
    months: u64,
    days_of_week: u64,
    dom_wildcard: bool,
    dow_wildcard: bool,

    pub static fn parse(expr: []const u8): CronError!CronExpr {
        const trimmed = std.mem.trim(u8, expr, " \t\r\n");
        if (std.mem.eql(u8, trimmed, "@yearly") or std.mem.eql(u8, trimmed, "@annually")) {
            return CronExpr.parse("0 0 1 1 *");
        }
        if (std.mem.eql(u8, trimmed, "@monthly")) { return CronExpr.parse("0 0 1 * *"); }
        if (std.mem.eql(u8, trimmed, "@weekly")) { return CronExpr.parse("0 0 * * 0"); }
        if (std.mem.eql(u8, trimmed, "@daily") or std.mem.eql(u8, trimmed, "@midnight")) {
            return CronExpr.parse("0 0 * * *");
        }
        if (std.mem.eql(u8, trimmed, "@hourly")) { return CronExpr.parse("0 * * * *"); }

        let fields: [5][]const u8 = undefined;
        let count: usize = 0;
        let tok_it = std.mem.tokenizeAny(u8, trimmed, " \t");
        while (true) {
            const tok = tok_it.next() orelse break;
            if (count >= 5) { return CronError.InvalidFieldCount; }
            fields[count] = tok;
            count += 1;
        }
        if (count != 5) { return CronError.InvalidFieldCount; }

        return CronExpr{
            .minutes = try parseBitmask(fields[0], 0, 59, false, false),
            .hours = try parseBitmask(fields[1], 0, 23, false, false),
            .days_of_month = try parseBitmask(fields[2], 1, 31, false, false),
            .months = try parseBitmask(fields[3], 1, 12, true, false),
            .days_of_week = try parseBitmask(fields[4], 0, 6, false, true),
            .dom_wildcard = std.mem.eql(u8, fields[2], "*"),
            .dow_wildcard = std.mem.eql(u8, fields[4], "*"),
        };
    }

    pub fn matches(dt: DateTime): bool {
        const min_bit: u6 = @intCast(dt.minute);
        const hr_bit: u6 = @intCast(dt.hour);
        const dom_bit: u6 = @intCast(dt.day);
        const mon_bit: u6 = @intCast(dt.month);
        const dow_bit: u6 = @intCast(dt.weekday());

        if ((_.minutes & (@as(u64, 1) << min_bit)) == 0) { return false; }
        if ((_.hours & (@as(u64, 1) << hr_bit)) == 0) { return false; }
        if ((_.months & (@as(u64, 1) << mon_bit)) == 0) { return false; }

        const dom_ok = (_.days_of_month & (@as(u64, 1) << dom_bit)) != 0;
        const dow_ok = (_.days_of_week & (@as(u64, 1) << dow_bit)) != 0;

        // Standard POSIX cron semantics: if both DOM and DOW are restricted, match either (OR)
        if (!_.dom_wildcard and !_.dow_wildcard) {
            return dom_ok or dow_ok;
        }
        return dom_ok and dow_ok;
    }

    /// Find the next trigger strictly after `start_dt` (up to 4 years lookahead)
    pub fn nextAfter(start_dt: DateTime): DateTime? {
        let cur: DateTime = DateTime{
            .year = start_dt.year,
            .month = start_dt.month,
            .day = start_dt.day,
            .hour = start_dt.hour,
            .minute = start_dt.minute,
        };
        cur = cur.addOneMinute();
        const max_steps: usize = 366 * 4 * 24 * 60;
        let step: usize = 0;
        while (step < max_steps) : (step += 1) {
            if (_.matches(cur)) { return cur; }
            cur = cur.addOneMinute();
        }
        return null;
    }
};
