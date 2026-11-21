// Source: Absorbed from /data/dev/bun/src/time.zig | Synced: 2026-11-21T14:57:50Z (RFC 3339 / ISO 8601 Timestamp Parser & Formatter)

/// 判断公历年份是否为闰年
pub fn isLeapYear(year: u16): bool {
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

/// 返回指定公历年月的天数（非法月份返回 0）
pub fn daysInMonth(year: u16, month: u8): u8 {
    return switch (month) {
        1, 3, 5, 7, 8, 10, 12 => 31,
        4, 6, 9, 11 => 30,
        2 => if (isLeapYear(year)) 29 else 28,
        else => 0,
    };
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn parse2Digits(s: []const u8, idx: usize): u8? {
    if (idx + 2 > s.len) { return null; }
    if (!isDigit(s[idx]) || !isDigit(s[idx + 1])) { return null; }
    return (s[idx] - '0') * 10 + (s[idx + 1] - '0');
}

fn parse4Digits(s: []const u8, idx: usize): u16? {
    if (idx + 4 > s.len) { return null; }
    if (!isDigit(s[idx]) || !isDigit(s[idx + 1]) || !isDigit(s[idx + 2]) || !isDigit(s[idx + 3])) {
        return null;
    }
    return @as(u16, s[idx] - '0') * 1000 +
        @as(u16, s[idx + 1] - '0') * 100 +
        @as(u16, s[idx + 2] - '0') * 10 +
        @as(u16, s[idx + 3] - '0');
}

/// Howard Hinnant 纯整数算法：公历日期 -> 距 1970-01-01 的有符号天数
fn daysFromCivil(y: i64, m: u8, d: u8): i64 {
    const y_adj: i64 = if (m <= 2) y - 1 else y;
    const era: i64 = @divFloor(y_adj, 400);
    const yoe: u64 = @intCast(y_adj - era * 400);
    const m_adj: u64 = if (m > 2) @as(u64, m) - 3 else @as(u64, m) + 9;
    const doy: u64 = (m_adj * 153 + 2) / 5 + @as(u64, d) - 1;
    const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + @as(i64, @intCast(doe)) - 719468;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn write2(*_, v: u8) {
        _.writeByte((v / 10) + '0');
        _.writeByte((v % 10) + '0');
    }

    fn write4(*_, v: u16) {
        _.writeByte(@intCast(((v / 1000) % 10) + '0'));
        _.writeByte(@intCast(((v / 100) % 10) + '0'));
        _.writeByte(@intCast(((v / 10) % 10) + '0'));
        _.writeByte(@intCast((v % 10) + '0'));
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// RFC 3339 / ISO 8601 时间戳结构体（零堆分配）
pub struct Rfc3339Time {
    year: u16 = 1970,
    month: u8 = 1,
    day: u8 = 1,
    hour: u8 = 0,
    minute: u8 = 0,
    second: u8 = 0,
    nanos: u32 = 0,
    offset_minutes: i16 = 0,

    /// 解析 RFC 3339 时间戳字符串（支持 `T`/`t`/` `、小数秒 `.1`..`.9` 与 `Z`/`z`/`+HH:MM`/`-HH:MM`）
    static fn parse(s: []const u8): Rfc3339Time? {
        if (s.len < 20) { return null; }

        const year = parse4Digits(s, 0) ?? return null;
        if (s[4] != '-') { return null; }
        const month = parse2Digits(s, 5) ?? return null;
        if (s[7] != '-') { return null; }
        const day = parse2Digits(s, 8) ?? return null;

        if (month < 1 || month > 12) { return null; }
        const max_day = daysInMonth(year, month);
        if (day < 1 || day > max_day) { return null; }

        const sep = s[10];
        if (sep != 'T' && sep != 't' && sep != ' ') { return null; }

        const hour = parse2Digits(s, 11) ?? return null;
        if (s[13] != ':') { return null; }
        const minute = parse2Digits(s, 14) ?? return null;
        if (s[16] != ':') { return null; }
        const second = parse2Digits(s, 17) ?? return null;

        if (hour > 23 || minute > 59 || second > 59) {
            return null;
        }

        let pos: usize = 19;
        let nanos: u32 = 0;

        // 可选小数秒 `.1`..`.9`（超过 9 位仅校验数字并截断至纳秒）
        if (pos < s.len && s[pos] == '.') {
            pos += 1;
            let frac_digits: usize = 0;
            let scale: u32 = 100000000;
            while (pos < s.len && isDigit(s[pos])) {
                if (frac_digits < 9) {
                    nanos += @as(u32, s[pos] - '0') * scale;
                    scale /= 10;
                }
                frac_digits += 1;
                pos += 1;
            }
            if (frac_digits == 0) {
                return null;
            }
        }

        // 必选时区后缀: Z / z / +HH:MM / -HH:MM
        if (pos >= s.len) { return null; }
        let offset_minutes: i16 = 0;
        const tz = s[pos];
        if (tz == 'Z' || tz == 'z') {
            if (pos + 1 != s.len) { return null; }
            offset_minutes = 0;
        } else if (tz == '+' || tz == '-') {
            if (pos + 6 != s.len) { return null; }
            const tz_h = parse2Digits(s, pos + 1) ?? return null;
            if (s[pos + 3] != ':') { return null; }
            const tz_m = parse2Digits(s, pos + 4) ?? return null;
            if (tz_h > 23 || tz_m > 59) { return null; }
            const mag: i16 = @as(i16, tz_h) * 60 + @as(i16, tz_m);
            offset_minutes = if (tz == '-') (0 - mag) else mag;
        } else {
            return null;
        }

        return Rfc3339Time{
            year: year,
            month: month,
            day: day,
            hour: hour,
            minute: minute,
            second: second,
            nanos: nanos,
            offset_minutes: offset_minutes,
        };
    }

    /// 转换为 UTC Unix 秒级时间戳（支持 1970 年前负时间戳）
    fn toUnixSec(): i64 {
        const days = daysFromCivil(@as(i64, _.year), _.month, _.day);
        const local_sec: i64 = days * 86400 +
            @as(i64, _.hour) * 3600 +
            @as(i64, _.minute) * 60 +
            @as(i64, _.second);
        return local_sec - @as(i64, _.offset_minutes) * 60;
    }

    /// 从 UTC Unix 秒级时间戳与纳秒还原 `Rfc3339Time`（Howard Hinnant civil-from-days，支持负时间戳）
    static fn fromUnixSecUtc(unix_sec: i64, nanos: u32): Rfc3339Time? {
        if (nanos >= 1000000000) { return null; }

        const days: i64 = @divFloor(unix_sec, 86400);
        const sec_of_day: u64 = @intCast(unix_sec - days * 86400);

        const z: i64 = days + 719468;
        const era: i64 = @divFloor(z, 146097);
        const doe: u64 = @intCast(z - era * 146097);
        const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        const y_base: i64 = @as(i64, @intCast(yoe)) + era * 400;
        const doy: u64 = doe - (yoe * 365 + yoe / 4 - yoe / 100);
        const mp: u64 = (doy * 5 + 2) / 153;
        const day: u8 = @intCast(doy - (mp * 153 + 2) / 5 + 1);
        const month: u8 = @intCast(if (mp < 10) mp + 3 else mp - 9);
        const year_i64: i64 = if (month <= 2) y_base + 1 else y_base;

        if (year_i64 < 0 || year_i64 > 9999) {
            return null;
        }

        const hour: u8 = @intCast(sec_of_day / 3600);
        const minute: u8 = @intCast((sec_of_day % 3600) / 60);
        const second: u8 = @intCast(sec_of_day % 60);

        return Rfc3339Time{
            year: @intCast(year_i64),
            month: month,
            day: day,
            hour: hour,
            minute: minute,
            second: second,
            nanos: nanos,
            offset_minutes: 0,
        };
    }

    /// 规范化格式化为 RFC 3339 字符串写入 `out`（如 `"2026-11-22T01:55:00Z"` 或含小数秒/时区偏移）
    fn format(out: []u8): ([]const u8)? {
        if (_.month < 1 || _.month > 12) { return null; }
        if (_.day < 1 || _.day > daysInMonth(_.year, _.month)) { return null; }
        if (_.hour > 23 || _.minute > 59 || _.second > 59 || _.nanos >= 1000000000) { return null; }
        if (_.offset_minutes < -1439 || _.offset_minutes > 1439) { return null; }

        let w = OutWriter{ buf: out };
        w.write4(_.year);
        w.writeByte('-');
        w.write2(_.month);
        w.writeByte('-');
        w.write2(_.day);
        w.writeByte('T');
        w.write2(_.hour);
        w.writeByte(':');
        w.write2(_.minute);
        w.writeByte(':');
        w.write2(_.second);

        if (_.nanos > 0) {
            let fbuf: [9]u8 = [0; 9];
            let rem = _.nanos;
            let k: usize = 9;
            while (k > 0) {
                k -= 1;
                fbuf[k] = @intCast((rem % 10) + '0');
                rem /= 10;
            }
            let flen: usize = 9;
            while (flen > 0 && fbuf[flen - 1] == '0') {
                flen -= 1;
            }
            w.writeByte('.');
            let fi: usize = 0;
            while (fi < flen) {
                w.writeByte(fbuf[fi]);
                fi += 1;
            }
        }

        if (_.offset_minutes == 0) {
            w.writeByte('Z');
        } else {
            const is_neg = _.offset_minutes < 0;
            const abs_min: u16 = @intCast(if (is_neg) (0 - _.offset_minutes) else _.offset_minutes);
            const tz_h: u8 = @intCast(abs_min / 60);
            const tz_m: u8 = @intCast(abs_min % 60);
            w.writeByte(if (is_neg) '-' else '+');
            w.write2(tz_h);
            w.writeByte(':');
            w.write2(tz_m);
        }

        return w.finish();
    }
}
