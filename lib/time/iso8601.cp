// Source: cp/lib/time/iso8601.cp (RFC 3339 / ISO 8601 DateTime & Duration)
// lib/time/iso8601.cp
// Pure CP RFC 3339 / ISO 8601 DateTime & Duration parser, formatter, and Howard Hinnant epoch conversion

pub struct CivilDateTime {
    year: i32 = 1970;
    month: u8 = 1;
    day: u8 = 1;
    hour: u8 = 0;
    minute: u8 = 0;
    second: u8 = 0;
    nanosecond: u32 = 0;
    tz_offset_minutes: i16 = 0;

    static fn init(
        year: i32,
        month: u8,
        day: u8,
        hour: u8,
        minute: u8,
        second: u8,
        nanosecond: u32 = 0,
        tz_offset_minutes: i16 = 0,
    ): CivilDateTime {
        return CivilDateTime {
            year: year,
            month: month,
            day: day,
            hour: hour,
            minute: minute,
            second: second,
            nanosecond: nanosecond,
            tz_offset_minutes: tz_offset_minutes,
        };
    }

    static fn utc(year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8): CivilDateTime {
        return CivilDateTime {
            year: year,
            month: month,
            day: day,
            hour: hour,
            minute: minute,
            second: second,
            nanosecond: 0,
            tz_offset_minutes: 0,
        };
    }

    static fn isLeapYear(year: i32): bool {
        const y = year as i64;
        return @mod(y, 4) == 0 && (@mod(y, 100) != 0 || @mod(y, 400) == 0);
    }

    static fn daysInMonth(year: i32, month: u8): u8 {
        return switch (month) {
            1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
            4 | 6 | 9 | 11 => 30,
            2 => if (CivilDateTime.isLeapYear(year)) (29 as u8) else (28 as u8),
            else => 0,
        };
    }

    /// Howard Hinnant `days_from_civil`: returns days since 1970-01-01 (works for all proleptic Gregorian dates).
    static fn daysFromCivil(year: i32, month: u8, day: u8): i64 {
        const m = month as i64;
        const d = day as i64;
        const y_adj: i64 = if (m <= 2) 1 else 0;
        const y: i64 = (year as i64) - y_adj;
        const era: i64 = @divFloor(y, 400);
        const yoe: i64 = y - era * 400;
        const m_adj: i64 = m + (if (m > 2) (-3 as i64) else (9 as i64));
        const doy: i64 = @divTrunc(153 * m_adj + 2, 5) + d - 1;
        const doe: i64 = yoe * 365 + @divTrunc(yoe, 4) - @divTrunc(yoe, 100) + doy;
        return era * 146097 + doe - 719468;
    }

    /// Howard Hinnant `civil_from_days`: converts days since 1970-01-01 to UTC CivilDateTime (00:00:00).
    static fn civilFromDays(z_days: i64): CivilDateTime {
        const z: i64 = z_days + 719468;
        const era: i64 = @divFloor(z, 146097);
        const doe: i64 = z - era * 146097;
        const yoe: i64 = @divTrunc(
            doe - @divTrunc(doe, 1460) + @divTrunc(doe, 36524) - @divTrunc(doe, 146096),
            365,
        );
        const y: i64 = yoe + era * 400;
        const doy: i64 = doe - (365 * yoe + @divTrunc(yoe, 4) - @divTrunc(yoe, 100));
        const mp: i64 = @divTrunc(5 * doy + 2, 153);
        const d: i64 = doy - @divTrunc(153 * mp + 2, 5) + 1;
        const m_off: i64 = if (mp < 10) 3 else -9;
        const m: i64 = mp + m_off;
        const final_adj: i64 = if (m <= 2) 1 else 0;
        const final_y: i64 = y + final_adj;
        return CivilDateTime {
            year: final_y as i32,
            month: m as u8,
            day: d as u8,
            hour: 0,
            minute: 0,
            second: 0,
            nanosecond: 0,
            tz_offset_minutes: 0,
        };
    }

    static fn fromUnixSeconds(epoch_sec: i64): CivilDateTime {
        const days = @divFloor(epoch_sec, 86400);
        const rem_sec = @mod(epoch_sec, 86400);
        let dt = CivilDateTime.civilFromDays(days);
        dt.hour = (@divTrunc(rem_sec, 3600)) as u8;
        const rem_after_h = @mod(rem_sec, 3600);
        dt.minute = (@divTrunc(rem_after_h, 60)) as u8;
        dt.second = (@mod(rem_after_h, 60)) as u8;
        return dt;
    }

    static fn fromUnixMillis(epoch_ms: i64): CivilDateTime {
        const sec = @divFloor(epoch_ms, 1000);
        const rem_ms = @mod(epoch_ms, 1000);
        let dt = CivilDateTime.fromUnixSeconds(sec);
        dt.nanosecond = (rem_ms as u32) * 1000000;
        return dt;
    }

    static fn fromUnixSecondsWithOffset(epoch_sec: i64, tz_offset_minutes: i16): CivilDateTime {
        const shifted = epoch_sec + (tz_offset_minutes as i64) * 60;
        let dt = CivilDateTime.fromUnixSeconds(shifted);
        dt.tz_offset_minutes = tz_offset_minutes;
        return dt;
    }

    static fn fromUnixMillisWithOffset(epoch_ms: i64, tz_offset_minutes: i16): CivilDateTime {
        const shifted = epoch_ms + (tz_offset_minutes as i64) * 60000;
        let dt = CivilDateTime.fromUnixMillis(shifted);
        dt.tz_offset_minutes = tz_offset_minutes;
        return dt;
    }

    fn isValid(): bool {
        if (_.month < 1 || _.month > 12) { return false; }
        const max_day = CivilDateTime.daysInMonth(_.year, _.month);
        if (_.day < 1 || _.day > max_day) { return false; }
        if (_.hour > 23 || _.minute > 59 || _.second > 59) { return false; }
        if (_.nanosecond >= 1000000000) { return false; }
        if (_.tz_offset_minutes < -1439 || _.tz_offset_minutes > 1439) { return false; }
        return true;
    }

    fn toUnixSeconds(): i64 {
        const days = CivilDateTime.daysFromCivil(_.year, _.month, _.day);
        const local_sec = days * 86400
            + (_.hour as i64) * 3600
            + (_.minute as i64) * 60
            + (_.second as i64);
        return local_sec - (_.tz_offset_minutes as i64) * 60;
    }

    fn toUnixMillis(): i64 {
        return _.toUnixSeconds() * 1000 + ((_.nanosecond / 1000000) as i64);
    }

    /// Returns day of week in POSIX convention: 0 = Sunday, 1 = Monday, ..., 6 = Saturday.
    fn dayOfWeek(): u8 {
        const days = CivilDateTime.daysFromCivil(_.year, _.month, _.day);
        // 1970-01-01 (days = 0) was Thursday (4)
        return @mod(days + 4, 7) as u8;
    }

    fn toUtc(): CivilDateTime {
        let dt = CivilDateTime.fromUnixSeconds(_.toUnixSeconds());
        dt.nanosecond = _.nanosecond;
        return dt;
    }

    fn withOffset(tz_offset_minutes: i16): CivilDateTime {
        let dt = CivilDateTime.fromUnixSecondsWithOffset(_.toUnixSeconds(), tz_offset_minutes);
        dt.nanosecond = _.nanosecond;
        return dt;
    }

    fn formatRfc3339(out: []u8): ([]const u8)! {
        return Iso8601.formatRfc3339(_, out);
    }
}

pub static struct Iso8601 {
    priv fn parseFixedDigits(s: []const u8): u32? {
        if (s.len == 0) { return null; }
        let acc: u32 = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') { return null; }
            acc = acc * 10 + ((c - '0') as u32);
            i += 1;
        }
        return acc;
    }

    priv fn write2Digits(out: []u8, pos: usize, val: u8): usize {
        out[pos] = ('0' as u8) + (val / 10);
        out[pos + 1] = ('0' as u8) + (val % 10);
        return pos + 2;
    }

    priv fn write4Digits(out: []u8, pos: usize, val: i32): usize {
        const v = val as u32;
        out[pos] = ('0' as u8) + ((v / 1000) % 10) as u8;
        out[pos + 1] = ('0' as u8) + ((v / 100) % 10) as u8;
        out[pos + 2] = ('0' as u8) + ((v / 10) % 10) as u8;
        out[pos + 3] = ('0' as u8) + (v % 10) as u8;
        return pos + 4;
    }

    /// Parses strict RFC 3339 / ISO 8601 DateTime: `YYYY-MM-DDTHH:MM:SS[.fff...][Z|+HH:MM|-HH:MM]`.
    fn parseDateTime(input: []const u8): CivilDateTime? {
        // Minimum length: `YYYY-MM-DDTHH:MM:SSZ` = 20 bytes
        if (input.len < 20) { return null; }
        if (input[4] != '-' || input[7] != '-') { return null; }
        if (input[10] != 'T' && input[10] != 't') { return null; }
        if (input[13] != ':' || input[16] != ':') { return null; }

        const yr = Iso8601.parseFixedDigits(input[0..4]) ?? return null;
        const mo = Iso8601.parseFixedDigits(input[5..7]) ?? return null;
        const dy = Iso8601.parseFixedDigits(input[8..10]) ?? return null;
        const hh = Iso8601.parseFixedDigits(input[11..13]) ?? return null;
        const mm = Iso8601.parseFixedDigits(input[14..16]) ?? return null;
        const ss = Iso8601.parseFixedDigits(input[17..19]) ?? return null;

        if (mo < 1 || mo > 12) { return null; }
        const max_day = CivilDateTime.daysInMonth(yr as i32, mo as u8);
        if (dy < 1 || dy > (max_day as u32)) { return null; }
        if (hh > 23 || mm > 59 || ss > 59) { return null; }

        let pos: usize = 19;
        let ns: u32 = 0;
        if (pos < input.len && input[pos] == '.') {
            pos += 1;
            const frac_start = pos;
            let mult: u32 = 100000000;
            while (pos < input.len && input[pos] >= '0' && input[pos] <= '9') {
                if (mult > 0) {
                    ns += ((input[pos] - '0') as u32) * mult;
                    mult /= 10;
                }
                pos += 1;
            }
            if (pos == frac_start) { return null; }
        }

        if (pos >= input.len) { return null; }

        const tz = input[pos..];
        let offset_mins: i16 = 0;
        if (tz.len == 1 && (tz[0] == 'Z' || tz[0] == 'z')) {
            offset_mins = 0;
        } else if (tz.len == 6 && (tz[0] == '+' || tz[0] == '-') && tz[3] == ':') {
            const oh = Iso8601.parseFixedDigits(tz[1..3]) ?? return null;
            const om = Iso8601.parseFixedDigits(tz[4..6]) ?? return null;
            if (oh > 23 || om > 59) { return null; }
            const total = ((oh * 60 + om) as i16);
            offset_mins = if (tz[0] == '-') -total else total;
        } else {
            return null;
        }

        return CivilDateTime {
            year: yr as i32,
            month: mo as u8,
            day: dy as u8,
            hour: hh as u8,
            minute: mm as u8,
            second: ss as u8,
            nanosecond: ns,
            tz_offset_minutes: offset_mins,
        };
    }

    /// Parses strict ISO 8601 calendar date `YYYY-MM-DD` as midnight UTC.
    fn parseDate(input: []const u8): CivilDateTime? {
        if (input.len != 10 || input[4] != '-' || input[7] != '-') { return null; }
        const yr = Iso8601.parseFixedDigits(input[0..4]) ?? return null;
        const mo = Iso8601.parseFixedDigits(input[5..7]) ?? return null;
        const dy = Iso8601.parseFixedDigits(input[8..10]) ?? return null;
        if (mo < 1 || mo > 12) { return null; }
        const max_day = CivilDateTime.daysInMonth(yr as i32, mo as u8);
        if (dy < 1 || dy > (max_day as u32)) { return null; }
        return CivilDateTime.utc(yr as i32, mo as u8, dy as u8, 0, 0, 0);
    }

    /// Formats a `CivilDateTime` into canonical RFC 3339 (`YYYY-MM-DDTHH:MM:SS[.fff][Z|+HH:MM|-HH:MM]`).
    fn formatRfc3339(dt: CivilDateTime, out: []u8): ([]const u8)! {
        if (dt.year < 0 || dt.year > 9999 || !dt.isValid()) {
            return error.InvalidDateTime;
        }

        // Minimum base: `YYYY-MM-DDTHH:MM:SS` = 19 bytes
        if (out.len < 20) { return error.BufferTooSmall; }

        let pos: usize = 0;
        pos = Iso8601.write4Digits(out, pos, dt.year);
        out[pos] = '-';
        pos += 1;
        pos = Iso8601.write2Digits(out, pos, dt.month);
        out[pos] = '-';
        pos += 1;
        pos = Iso8601.write2Digits(out, pos, dt.day);
        out[pos] = 'T';
        pos += 1;
        pos = Iso8601.write2Digits(out, pos, dt.hour);
        out[pos] = ':';
        pos += 1;
        pos = Iso8601.write2Digits(out, pos, dt.minute);
        out[pos] = ':';
        pos += 1;
        pos = Iso8601.write2Digits(out, pos, dt.second);

        if (dt.nanosecond > 0) {
            let digits: [9]u8 = [0; 9];
            let rem = dt.nanosecond;
            let idx: usize = 9;
            while (idx > 0) {
                idx -= 1;
                digits[idx] = ('0' as u8) + ((rem % 10) as u8);
                rem /= 10;
            }
            let frac_len: usize = 9;
            while (frac_len > 1 && digits[frac_len - 1] == '0') {
                frac_len -= 1;
            }
            if (pos + 1 + frac_len > out.len) { return error.BufferTooSmall; }
            out[pos] = '.';
            pos += 1;
            let j: usize = 0;
            while (j < frac_len) {
                out[pos + j] = digits[j];
                j += 1;
            }
            pos += frac_len;
        }

        if (dt.tz_offset_minutes == 0) {
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'Z';
            pos += 1;
        } else {
            if (pos + 6 > out.len) { return error.BufferTooSmall; }
            const sign: u8 = if (dt.tz_offset_minutes < 0) '-' else '+';
            const abs_m: u16 = if (dt.tz_offset_minutes < 0) ((0 - dt.tz_offset_minutes) as u16) else (dt.tz_offset_minutes as u16);
            const oh = (abs_m / 60) as u8;
            const om = (abs_m % 60) as u8;
            out[pos] = sign;
            pos += 1;
            pos = Iso8601.write2Digits(out, pos, oh);
            out[pos] = ':';
            pos += 1;
            pos = Iso8601.write2Digits(out, pos, om);
        }

        return out[0..pos];
    }
}

pub struct IsoDuration {
    negative: bool = false;
    years: i64 = 0;
    months: i64 = 0;
    weeks: i64 = 0;
    days: i64 = 0;
    hours: i64 = 0;
    minutes: i64 = 0;
    seconds: i64 = 0;
    millis: i64 = 0;

    priv static fn writeI64(out: []u8, start: usize, val: i64): usize? {
        let v: u64 = if (val < 0) ((0 - val) as u64) else (val as u64);
        let tmp: [20]u8 = [0; 20];
        let len: usize = 0;
        if (v == 0) {
            tmp[0] = '0';
            len = 1;
        } else {
            while (v > 0) {
                tmp[len] = ('0' as u8) + ((v % 10) as u8);
                len += 1;
                v /= 10;
            }
        }
        if (start + len > out.len) { return null; }
        let i: usize = 0;
        while (i < len) {
            out[start + i] = tmp[len - 1 - i];
            i += 1;
        }
        return start + len;
    }

    /// Parses an ISO 8601 duration string such as `P1DT2H30M15S`, `P2W`, `PT0.5S`, `-P1Y2M3DT4H5M6S`.
    static fn parse(input: []const u8): IsoDuration? {
        if (input.len < 3) { return null; }

        let pos: usize = 0;
        let neg = false;
        if (input[pos] == '-') {
            neg = true;
            pos += 1;
        } else if (input[pos] == '+') {
            pos += 1;
        }

        if (pos >= input.len || input[pos] != 'P') { return null; }
        pos += 1;
        if (pos >= input.len) { return null; }

        let dur = IsoDuration { negative: neg };
        let in_time = false;
        let any_component = false;
        let any_time_component = false;
        let date_stage: u8 = 0; // 0=start, 1=after Y, 2=after M, 3=after W, 4=after D
        let time_stage: u8 = 0; // 0=start, 1=after H, 2=after M, 3=after S

        while (pos < input.len) {
            if (input[pos] == 'T') {
                if (in_time) { return null; }
                in_time = true;
                pos += 1;
                if (pos >= input.len) { return null; }
                continue;
            }

            const num_start = pos;
            let val: i64 = 0;
            while (pos < input.len && input[pos] >= '0' && input[pos] <= '9') {
                const digit = (input[pos] - '0') as i64;
                if (val > 9000000000000000) { return null; }
                val = val * 10 + digit;
                pos += 1;
            }
            if (pos == num_start) { return null; }

            let frac_ms: i64 = 0;
            let has_frac = false;
            if (pos < input.len && (input[pos] == '.' || input[pos] == ',')) {
                has_frac = true;
                pos += 1;
                const frac_start = pos;
                let mult: u64 = 100;
                while (pos < input.len && input[pos] >= '0' && input[pos] <= '9') {
                    if (mult > 0) {
                        frac_ms += (((input[pos] - '0') as u64) * mult) as i64;
                        mult /= 10;
                    }
                    pos += 1;
                }
                if (pos == frac_start) { return null; }
            }

            if (pos >= input.len) { return null; }
            const designator = input[pos];
            pos += 1;

            if (!in_time) {
                if (has_frac) { return null; }
                if (designator == 'Y' && date_stage < 1) {
                    dur.years = val;
                    date_stage = 1;
                } else if (designator == 'M' && date_stage < 2) {
                    dur.months = val;
                    date_stage = 2;
                } else if (designator == 'W' && date_stage < 3) {
                    dur.weeks = val;
                    date_stage = 3;
                } else if (designator == 'D' && date_stage < 4) {
                    dur.days = val;
                    date_stage = 4;
                } else {
                    return null;
                }
            } else {
                if (has_frac && designator != 'S') { return null; }
                if (designator == 'H' && time_stage < 1) {
                    dur.hours = val;
                    time_stage = 1;
                } else if (designator == 'M' && time_stage < 2) {
                    dur.minutes = val;
                    time_stage = 2;
                } else if (designator == 'S' && time_stage < 3) {
                    dur.seconds = val;
                    dur.millis = frac_ms;
                    time_stage = 3;
                } else {
                    return null;
                }
                any_time_component = true;
            }
            any_component = true;
        }

        if (!any_component) { return null; }
        if (in_time && !any_time_component) { return null; }
        return dur;
    }

    /// Converts duration to milliseconds (uses 365d/year and 30d/month if calendar fields are non-zero).
    fn toMilliseconds(): i64 {
        const total = _.years * 365 * 86400000
            + _.months * 30 * 86400000
            + _.weeks * 7 * 86400000
            + _.days * 86400000
            + _.hours * 3600000
            + _.minutes * 60000
            + _.seconds * 1000
            + _.millis;
        return if (_.negative) -total else total;
    }

    /// Converts exact fixed-length durations to milliseconds; returns null if `years` or `months` are non-zero.
    fn toMillisecondsExact(): i64? {
        if (_.years != 0 || _.months != 0) { return null; }
        return _.toMilliseconds();
    }

    fn toSeconds(): i64 {
        return @divTrunc(_.toMilliseconds(), 1000);
    }

    fn format(out: []u8): ([]const u8)! {
        let pos: usize = 0;
        if (_.negative) {
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = '-';
            pos += 1;
        }
        if (pos + 1 > out.len) { return error.BufferTooSmall; }
        out[pos] = 'P';
        pos += 1;

        const has_date = _.years != 0 || _.months != 0 || _.weeks != 0 || _.days != 0;
        const has_time = _.hours != 0 || _.minutes != 0 || _.seconds != 0 || _.millis != 0;

        if (!has_date && !has_time) {
            if (pos + 3 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'T';
            out[pos + 1] = '0';
            out[pos + 2] = 'S';
            return out[0..(pos + 3)];
        }

        if (_.years != 0) {
            const np = IsoDuration.writeI64(out, pos, _.years) ?? return error.BufferTooSmall;
            pos = np;
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'Y';
            pos += 1;
        }
        if (_.months != 0) {
            const np = IsoDuration.writeI64(out, pos, _.months) ?? return error.BufferTooSmall;
            pos = np;
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'M';
            pos += 1;
        }
        if (_.weeks != 0) {
            const np = IsoDuration.writeI64(out, pos, _.weeks) ?? return error.BufferTooSmall;
            pos = np;
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'W';
            pos += 1;
        }
        if (_.days != 0) {
            const np = IsoDuration.writeI64(out, pos, _.days) ?? return error.BufferTooSmall;
            pos = np;
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'D';
            pos += 1;
        }

        if (has_time) {
            if (pos + 1 > out.len) { return error.BufferTooSmall; }
            out[pos] = 'T';
            pos += 1;

            if (_.hours != 0) {
                const np = IsoDuration.writeI64(out, pos, _.hours) ?? return error.BufferTooSmall;
                pos = np;
                if (pos + 1 > out.len) { return error.BufferTooSmall; }
                out[pos] = 'H';
                pos += 1;
            }
            if (_.minutes != 0) {
                const np = IsoDuration.writeI64(out, pos, _.minutes) ?? return error.BufferTooSmall;
                pos = np;
                if (pos + 1 > out.len) { return error.BufferTooSmall; }
                out[pos] = 'M';
                pos += 1;
            }
            if (_.seconds != 0 || _.millis != 0) {
                const np = IsoDuration.writeI64(out, pos, _.seconds) ?? return error.BufferTooSmall;
                pos = np;
                if (_.millis != 0) {
                    const m_abs: u64 = if (_.millis < 0) ((0 - _.millis) as u64) else (_.millis as u64);
                    let m_digits: [3]u8 = [
                        ('0' as u8) + (((m_abs / 100) % 10) as u8),
                        ('0' as u8) + (((m_abs / 10) % 10) as u8),
                        ('0' as u8) + ((m_abs % 10) as u8),
                    ];
                    let m_len: usize = 3;
                    while (m_len > 1 && m_digits[m_len - 1] == '0') {
                        m_len -= 1;
                    }
                    if (pos + 1 + m_len > out.len) { return error.BufferTooSmall; }
                    out[pos] = '.';
                    pos += 1;
                    let j: usize = 0;
                    while (j < m_len) {
                        out[pos + j] = m_digits[j];
                        j += 1;
                    }
                    pos += m_len;
                }
                if (pos + 1 > out.len) { return error.BufferTooSmall; }
                out[pos] = 'S';
                pos += 1;
            }
        }

        return out[0..pos];
    }
}
