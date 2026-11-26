import {
    IsoDate, IsoTime, IsoDateTime,
    parseDateTime, formatDateTime, isLeapYear
} from "../../iso8601/src/index.cp";

pub struct Rfc3339Timestamp {
    pub dt: IsoDateTime,
    pub unix_seconds: i64,
    pub millisecond: u16,
}

fn rfcIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

// Howard Hinnant civil-from-days / days-from-civil algorithm (exact Gregorian, O(1))
pub fn daysFromCivil(year: i32, month: u8, day: u8): i64 {
    let adj: i64 = if (month <= 2) { 1 } else { 0 };
    let y: i64 = (year as i64) - adj;
    let era: i64 = if (y >= 0) {
        ((y as u64) / 400) as i64
    } else {
        -(((((-y) + 399) as u64) / 400) as i64)
    };
    let yoe: u64 = (y - era * 400) as u64;
    let m: u64 = month as u64;
    let m_adj: u64 = if (m > 2) { m - 3 } else { m + 9 };
    let doy: u64 = (153 * m_adj + 2) / 5 + ((day as u64) - 1);
    let doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (doe as i64) - 719468;
}

pub fn civilFromDays(z_in: i64): IsoDate {
    let z: i64 = z_in + 719468;
    let era: i64 = if (z >= 0) {
        ((z as u64) / 146097) as i64
    } else {
        -(((((-z) + 146096) as u64) / 146097) as i64)
    };
    let doe: u64 = (z - era * 146097) as u64;
    let yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y: i64 = (yoe as i64) + era * 400;
    let doy: u64 = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp: u64 = (5 * doy + 2) / 153;
    let d: u8 = (doy - (153 * mp + 2) / 5 + 1) as u8;
    let m: u8 = (if (mp < 10) { mp + 3 } else { mp - 9 }) as u8;
    let m_adj: i64 = if (m <= 2) { 1 } else { 0 };
    let final_y: i32 = (y + m_adj) as i32;
    return IsoDate {
        year: final_y,
        month: m,
        day: d,
        ordinal: 0,
        week: 0,
        weekday: 0,
        kind: 0,
    };
}

// Strict RFC 3339 check: YYYY-MM-DD[T|t| ]HH:MM:SS(.frac)?(Z|z|[+-]HH:MM)
pub fn isValidRfc3339(s: string): bool {
    if (s.len < 20) { return false; }
    if (s[4] != 45 || s[7] != 45) { return false; } // '-'
    let sep = s[10];
    if (sep != 84 && sep != 116 && sep != 32) { return false; } // 'T','t',' '
    if (s[13] != 58 || s[16] != 58) { return false; } // ':'
    // Verify timezone colon when numeric offset is used
    let pos: usize = 19;
    if (pos < s.len && s[pos] == 46) { // '.'
        pos = pos + 1;
        let digits: usize = 0;
        while (pos < s.len && rfcIsDigit(s[pos])) {
            digits = digits + 1;
            pos = pos + 1;
        }
        if (digits == 0) { return false; }
    }
    if (pos >= s.len) { return false; }
    let tz = s[pos];
    if (tz == 90 || tz == 122) {
        return pos + 1 == s.len;
    }
    if (tz == 43 || tz == 45) {
        if (pos + 6 != s.len) { return false; }
        if (s[pos + 3] != 58) { return false; } // RFC 3339 requires colon in ±HH:MM
        return true;
    }
    return false;
}

pub fn parseRfc3339(s: string): Rfc3339Timestamp? {
    if (!isValidRfc3339(s)) {
        return null;
    }
    let dt_opt = parseDateTime(s);
    if (dt_opt == null) {
        return null;
    }
    let dt = dt_opt ?? IsoDateTime {
        date: IsoDate { year: 1970, month: 1, day: 1, ordinal: 0, week: 0, weekday: 0, kind: 0 },
        time: IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: true },
    };
    if (dt.date.kind != 0 || !dt.time.has_tz || dt.time.hour > 23) {
        return null;
    }
    let days = daysFromCivil(dt.date.year, dt.date.month, dt.date.day);
    let secs_of_day: i64 = (dt.time.hour as i64) * 3600 + (dt.time.minute as i64) * 60 + (dt.time.second as i64);
    let unix_secs = days * 86400 + secs_of_day - ((dt.time.tz_offset_minutes as i64) * 60);
    return Rfc3339Timestamp {
        dt: dt,
        unix_seconds: unix_secs,
        millisecond: dt.time.millisecond,
    };
}

pub fn fromUnixSeconds(unix_secs: i64): IsoDateTime {
    let days: i64 = if (unix_secs >= 0) {
        ((unix_secs as u64) / 86400) as i64
    } else {
        -(((((-unix_secs) + 86399) as u64) / 86400) as i64)
    };
    let rem: u64 = (unix_secs - days * 86400) as u64;
    let h = (rem / 3600) as u8;
    let m = ((rem % 3600) / 60) as u8;
    let s = (rem % 60) as u8;
    let d = civilFromDays(days);
    return IsoDateTime {
        date: d,
        time: IsoTime {
            hour: h,
            minute: m,
            second: s,
            millisecond: 0,
            tz_offset_minutes: 0,
            has_tz: true,
        },
    };
}

pub fn formatRfc3339Utc(unix_secs: i64, out: []u8): usize {
    let dt = fromUnixSeconds(unix_secs);
    return formatDateTime(dt, out);
}

pub fn isRfcLeapYear(year: i32): bool {
    return isLeapYear(year);
}
