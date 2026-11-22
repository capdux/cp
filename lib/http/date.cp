// Source: Absorbed from /data/dev/bun/src/http/Headers.cp | Synced: 2026-11-22T11:12:13Z (RFC 9110 §5.6.7 HTTP-date / IMF-fixdate)
// Pure-CP zero-allocation IMF-fixdate (`Sun, 06 Nov 1994 08:49:37 GMT`) parser
// and formatter with Howard Hinnant civil calendar conversion to/from Unix epoch.

pub struct HttpDateTime {
    year: i32,
    month: u8,    // 1..12
    day: u8,      // 1..31
    hour: u8,     // 0..23
    minute: u8,   // 0..59
    second: u8,   // 0..59
    weekday: u8,  // 0=Sun, 1=Mon, ..., 6=Sat

    /// Convert Unix timestamp (seconds since 1970-01-01T00:00:00Z) to UTC `HttpDateTime`.
    pub static fn fromUnixSeconds(epoch: i64): HttpDateTime {
        const days = floorDiv(epoch, 86400);
        const secs_in_day: u32 = @intCast(epoch - days * 86400);

        const hour: u8 = @intCast(secs_in_day / 3600);
        const rem_h = secs_in_day % 3600;
        const minute: u8 = @intCast(rem_h / 60);
        const second: u8 = @intCast(rem_h % 60);

        // Howard Hinnant civil_from_days
        const z = days + 719468;
        const era = floorDiv(z, 146097);
        const doe: u64 = @intCast(z - era * 146097); // [0, 146096]
        const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
        let y: i64 = @as(i64, @intCast(yoe)) + era * 400;
        const doy: u64 = doe - (yoe * 365 + yoe / 4 - yoe / 100); // [0, 365]
        const mp: u64 = (doy * 5 + 2) / 153; // [0, 11]
        const d: u8 = @intCast(doy - (mp * 153 + 2) / 5 + 1); // [1, 31]
        const m: u8 = @intCast(if (mp < 10) mp + 3 else mp - 9); // [1, 12]
        if (m <= 2) y += 1;

        // 1970-01-01 was Thursday (4)
        const wd_raw = @mod(days + 4, 7);
        const weekday: u8 = @intCast(if (wd_raw < 0) wd_raw + 7 else wd_raw);

        return HttpDateTime{
            year: @intCast(y),
            month: m,
            day: d,
            hour: hour,
            minute: minute,
            second: second,
            weekday: weekday,
        };
    }

    /// Convert UTC `HttpDateTime` to Unix timestamp (seconds since 1970-01-01T00:00:00Z).
    pub fn toUnixSeconds(): i64 {
        let y: i64 = _.year;
        const m: i64 = @as(i64, _.month);
        const d: i64 = @as(i64, _.day);
        if (m <= 2) y -= 1;
        const era = floorDiv(y, 400);
        const yoe: u64 = @intCast(y - era * 400);
        const m_adj: u64 = @intCast(if (m > 2) m - 3 else m + 9);
        const doy: u64 = (m_adj * 153 + 2) / 5 + @as(u64, @intCast(d)) - 1;
        const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        const days: i64 = era * 146097 + @as(i64, @intCast(doe)) - 719468;
        return days * 86400 + @as(i64, _.hour) * 3600 + @as(i64, _.minute) * 60 + @as(i64, _.second);
    }

    /// Format as RFC 9110 IMF-fixdate (`"Sun, 06 Nov 1994 08:49:37 GMT"`, exactly 29 bytes) into `out`.
    pub fn formatImf(out: []u8): ([]const u8)? {
        if (out.len < 29) return null;
        const wd = weekdayName(_.weekday);
        const mon = monthName(_.month);
        out[0] = wd[0];
        out[1] = wd[1];
        out[2] = wd[2];
        out[3] = ',';
        out[4] = ' ';
        write2Digits(out, 5, _.day);
        out[7] = ' ';
        out[8] = mon[0];
        out[9] = mon[1];
        out[10] = mon[2];
        out[11] = ' ';
        const y_u32: u32 = if (_.year < 0) 0 else @intCast(_.year);
        write4Digits(out, 12, y_u32);
        out[16] = ' ';
        write2Digits(out, 17, _.hour);
        out[19] = ':';
        write2Digits(out, 20, _.minute);
        out[22] = ':';
        write2Digits(out, 23, _.second);
        out[25] = ' ';
        out[26] = 'G';
        out[27] = 'M';
        out[28] = 'T';
        return out[0..29];
    }

    /// Parse an RFC 9110 IMF-fixdate (`"Sun, 06 Nov 1994 08:49:37 GMT"`).
    pub static fn parseImf(input: []const u8): HttpDateTime? {
        const s = trimOws(input);
        if (s.len != 29) return null;
        if (s[3] != ',' || s[4] != ' ' || s[7] != ' ' || s[11] != ' ' ||
            s[16] != ' ' || s[19] != ':' || s[22] != ':' || s[25] != ' ')
        {
            return null;
        }
        if (!strEq(s[26..29], "GMT")) return null;

        const wd = parseWeekday(s[0..3]) ?? return null;
        const day = parse2Digits(s[5..7]) ?? return null;
        const month = parseMonth(s[8..11]) ?? return null;
        const year_u = parse4Digits(s[12..16]) ?? return null;
        const hour = parse2Digits(s[17..19]) ?? return null;
        const minute = parse2Digits(s[20..22]) ?? return null;
        const second = parse2Digits(s[23..25]) ?? return null;

        if (day < 1 || day > daysInMonth(@intCast(year_u), month)) return null;
        if (hour > 23 || minute > 59 || second > 59) return null;

        return HttpDateTime{
            year: @intCast(year_u),
            month: month,
            day: day,
            hour: hour,
            minute: minute,
            second: second,
            weekday: wd,
        };
    }
}

/// Convenience helper: format Unix epoch seconds directly into IMF-fixdate string.
pub fn formatHttpDate(epoch_sec: i64, out: []u8): ([]const u8)? {
    const dt = HttpDateTime.fromUnixSeconds(epoch_sec);
    return dt.formatImf(out);
}

/// Convenience helper: parse IMF-fixdate directly into Unix epoch seconds.
pub fn parseHttpDate(input: []const u8): i64? {
    const dt = HttpDateTime.parseImf(input) ?? return null;
    return dt.toUnixSeconds();
}

// ---------- Helpers ----------

fn floorDiv(a: i64, b: i64): i64 {
    const q = @divTrunc(a, b);
    const r = @rem(a, b);
    return if (r < 0) q - 1 else q;
}

fn isLeapYear(y: i32): bool {
    return (@rem(y, 4) == 0 && @rem(y, 100) != 0) || (@rem(y, 400) == 0);
}

fn daysInMonth(y: i32, m: u8): u8 {
    return switch (m) {
        1, 3, 5, 7, 8, 10, 12 => 31,
        4, 6, 9, 11 => 30,
        2 => if (isLeapYear(y)) 29 else 28,
        else => 0,
    };
}

fn weekdayName(wd: u8): []const u8 {
    return switch (wd) {
        0 => "Sun",
        1 => "Mon",
        2 => "Tue",
        3 => "Wed",
        4 => "Thu",
        5 => "Fri",
        6 => "Sat",
        else => "Sun",
    };
}

fn monthName(m: u8): []const u8 {
    return switch (m) {
        1 => "Jan",
        2 => "Feb",
        3 => "Mar",
        4 => "Apr",
        5 => "May",
        6 => "Jun",
        7 => "Jul",
        8 => "Aug",
        9 => "Sep",
        10 => "Oct",
        11 => "Nov",
        12 => "Dec",
        else => "Jan",
    };
}

fn parseWeekday(s: []const u8): u8? {
    if (strEq(s, "Sun")) return 0;
    if (strEq(s, "Mon")) return 1;
    if (strEq(s, "Tue")) return 2;
    if (strEq(s, "Wed")) return 3;
    if (strEq(s, "Thu")) return 4;
    if (strEq(s, "Fri")) return 5;
    if (strEq(s, "Sat")) return 6;
    return null;
}

fn parseMonth(s: []const u8): u8? {
    if (strEq(s, "Jan")) return 1;
    if (strEq(s, "Feb")) return 2;
    if (strEq(s, "Mar")) return 3;
    if (strEq(s, "Apr")) return 4;
    if (strEq(s, "May")) return 5;
    if (strEq(s, "Jun")) return 6;
    if (strEq(s, "Jul")) return 7;
    if (strEq(s, "Aug")) return 8;
    if (strEq(s, "Sep")) return 9;
    if (strEq(s, "Oct")) return 10;
    if (strEq(s, "Nov")) return 11;
    if (strEq(s, "Dec")) return 12;
    return null;
}

fn parse2Digits(s: []const u8): u8? {
    if (s.len != 2 || s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') return null;
    return (s[0] - '0') * 10 + (s[1] - '0');
}

fn parse4Digits(s: []const u8): u32? {
    if (s.len != 4) return null;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + @as(u32, s[i] - '0');
    }
    return acc;
}

fn write2Digits(out: []u8, pos: usize, v: u8) {
    out[pos] = @as(u8, '0') + (v / 10);
    out[pos + 1] = @as(u8, '0') + (v % 10);
}

fn write4Digits(out: []u8, pos: usize, v: u32) {
    out[pos] = @intCast('0' + ((v / 1000) % 10));
    out[pos + 1] = @intCast('0' + ((v / 100) % 10));
    out[pos + 2] = @intCast('0' + ((v / 10) % 10));
    out[pos + 3] = @intCast('0' + (v % 10));
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
