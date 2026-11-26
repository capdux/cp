// @cap/http-date — Zero-allocation RFC 9110 §5.6.7 HTTP-date Parser & Formatter
// Clean-room MIT implementation in pure CP.
// Supports IMF-fixdate, RFC 850, and ANSI C asctime() formats plus Hinnant civil/epoch math.

pub struct HttpDateTime {
    pub year: u16,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
    pub weekday: u8, // 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
    pub valid: bool,
}

fn invalidDateTime() -> HttpDateTime {
    return HttpDateTime {
        year: 0,
        month: 0,
        day: 0,
        hour: 0,
        minute: 0,
        second: 0,
        weekday: 0,
        valid: false,
    };
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn isLeapYear(y: u16) -> bool {
    const yu = y as u64;
    return (yu % 4 == 0) && ((yu % 100 != 0) || (yu % 400 == 0));
}

fn daysInMonth(y: u16, m: u8) -> u8 {
    if (m == 2) {
        if (isLeapYear(y)) {
            return 29;
        }
        return 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return 30;
    }
    if (m >= 1 && m <= 12) {
        return 31;
    }
    return 0;
}

fn daysFromCivil(year: u16, month: u8, day: u8) -> u64 {
    let y = year as u64;
    const m = month as u64;
    const d = day as u64;
    if (m <= 2) {
        y -= 1;
    }
    const era = y / 400;
    const yoe = y - era * 400;
    let mp: u64 = 0;
    if (m > 2) {
        mp = m - 3;
    } else {
        mp = m + 9;
    }
    const doy: u64 = (mp * 153 + 2) / 5 + d - 1;
    const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

fn buildValidated(year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) -> HttpDateTime {
    if (year < 1970 || year > 9999 || month < 1 || month > 12) {
        return invalidDateTime();
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return invalidDateTime();
    }
    const max_d = daysInMonth(year as u16, month as u8) as i32;
    if (day < 1 || day > max_d) {
        return invalidDateTime();
    }
    const days = daysFromCivil(year as u16, month as u8, day as u8);
    const wd = ((days + 4) % 7) as u8; // 1970-01-01 was Thursday (4)
    return HttpDateTime {
        year: year as u16,
        month: month as u8,
        day: day as u8,
        hour: hour as u8,
        minute: minute as u8,
        second: second as u8,
        weekday: wd,
        valid: true,
    };
}

fn parse2Digits(s: string, pos: usize) -> i32 {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    if (c0 < 48 || c0 > 57 || c1 < 48 || c1 > 57) {
        return -1;
    }
    return ((c0 - 48) as i32) * 10 + ((c1 - 48) as i32);
}

fn parse4Digits(s: string, pos: usize) -> i32 {
    const hi = parse2Digits(s, pos);
    const lo = parse2Digits(s, pos + 2);
    if (hi < 0 || lo < 0) {
        return -1;
    }
    return hi * 100 + lo;
}

fn parseMonth(s: string, pos: usize) -> i32 {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    const c2 = s[pos + 2];
    if (c0 == 74 && c1 == 97 && c2 == 110) { return 1; }  // Jan
    if (c0 == 70 && c1 == 101 && c2 == 98) { return 2; }  // Feb
    if (c0 == 77 && c1 == 97 && c2 == 114) { return 3; }  // Mar
    if (c0 == 65 && c1 == 112 && c2 == 114) { return 4; } // Apr
    if (c0 == 77 && c1 == 97 && c2 == 121) { return 5; }  // May
    if (c0 == 74 && c1 == 117 && c2 == 110) { return 6; } // Jun
    if (c0 == 74 && c1 == 117 && c2 == 108) { return 7; } // Jul
    if (c0 == 65 && c1 == 117 && c2 == 103) { return 8; } // Aug
    if (c0 == 83 && c1 == 101 && c2 == 112) { return 9; } // Sep
    if (c0 == 79 && c1 == 99 && c2 == 116) { return 10; } // Oct
    if (c0 == 78 && c1 == 111 && c2 == 118) { return 11; }// Nov
    if (c0 == 68 && c1 == 101 && c2 == 99) { return 12; } // Dec
    return -1;
}

fn isShortWeekday(s: string, pos: usize) -> bool {
    const c0 = s[pos];
    const c1 = s[pos + 1];
    const c2 = s[pos + 2];
    if (c0 == 83 && c1 == 117 && c2 == 110) { return true; } // Sun
    if (c0 == 77 && c1 == 111 && c2 == 110) { return true; } // Mon
    if (c0 == 84 && c1 == 117 && c2 == 101) { return true; } // Tue
    if (c0 == 87 && c1 == 101 && c2 == 100) { return true; } // Wed
    if (c0 == 84 && c1 == 104 && c2 == 117) { return true; } // Thu
    if (c0 == 70 && c1 == 114 && c2 == 105) { return true; } // Fri
    if (c0 == 83 && c1 == 97 && c2 == 116) { return true; }  // Sat
    return false;
}

fn sliceEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isLongWeekday(s: string) -> bool {
    return sliceEq(s, "Sunday")
        || sliceEq(s, "Monday")
        || sliceEq(s, "Tuesday")
        || sliceEq(s, "Wednesday")
        || sliceEq(s, "Thursday")
        || sliceEq(s, "Friday")
        || sliceEq(s, "Saturday");
}

fn parseImfFixdate(s: string) -> HttpDateTime {
    // "Sun, 06 Nov 1994 08:49:37 GMT" (29 bytes)
    if (s.len != 29 || !isShortWeekday(s, 0)) {
        return invalidDateTime();
    }
    if (s[3] != 44 || s[4] != 32 || s[7] != 32 || s[11] != 32 || s[16] != 32
        || s[19] != 58 || s[22] != 58
        || s[25] != 32 || s[26] != 71 || s[27] != 77 || s[28] != 84)
    {
        return invalidDateTime();
    }
    const day = parse2Digits(s, 5);
    const month = parseMonth(s, 8);
    const year = parse4Digits(s, 12);
    const hour = parse2Digits(s, 17);
    const minute = parse2Digits(s, 20);
    const second = parse2Digits(s, 23);
    return buildValidated(year, month, day, hour, minute, second);
}

fn parseRfc850(s: string) -> HttpDateTime {
    // "Sunday, 06-Nov-94 08:49:37 GMT"
    let comma: usize = 0;
    while (comma < s.len && s[comma] != 44) {
        comma += 1;
    }
    if (comma >= s.len || !isLongWeekday(s[0..comma])) {
        return invalidDateTime();
    }
    if (s.len - comma != 24 || s[comma + 1] != 32) {
        return invalidDateTime();
    }
    const p = comma + 2; // "06-Nov-94 08:49:37 GMT" (22 bytes)
    if (s[p + 2] != 45 || s[p + 6] != 45 || s[p + 9] != 32
        || s[p + 12] != 58 || s[p + 15] != 58
        || s[p + 18] != 32 || s[p + 19] != 71 || s[p + 20] != 77 || s[p + 21] != 84)
    {
        return invalidDateTime();
    }
    const day = parse2Digits(s, p);
    const month = parseMonth(s, p + 3);
    const yy = parse2Digits(s, p + 7);
    if (yy < 0) {
        return invalidDateTime();
    }
    let year: i32 = 2000 + yy;
    if (yy >= 70) {
        year = 1900 + yy;
    }
    const hour = parse2Digits(s, p + 10);
    const minute = parse2Digits(s, p + 13);
    const second = parse2Digits(s, p + 16);
    return buildValidated(year, month, day, hour, minute, second);
}

fn parseAsctime(s: string) -> HttpDateTime {
    // "Sun Nov  6 08:49:37 1994" (24 bytes)
    if (s.len != 24 || !isShortWeekday(s, 0)) {
        return invalidDateTime();
    }
    if (s[3] != 32 || s[7] != 32 || s[10] != 32
        || s[13] != 58 || s[16] != 58 || s[19] != 32)
    {
        return invalidDateTime();
    }
    const month = parseMonth(s, 4);
    let day: i32 = -1;
    if (s[8] == 32) {
        if (s[9] >= 48 && s[9] <= 57) {
            day = (s[9] - 48) as i32;
        }
    } else {
        day = parse2Digits(s, 8);
    }
    const hour = parse2Digits(s, 11);
    const minute = parse2Digits(s, 14);
    const second = parse2Digits(s, 17);
    const year = parse4Digits(s, 20);
    return buildValidated(year, month, day, hour, minute, second);
}

/// Parses an RFC 9110 §5.6.7 HTTP-date string in any of the 3 standard formats:
/// IMF-fixdate, obsolete RFC 850, or ANSI C `asctime()`.
pub fn parseHttpDate(input: string) -> HttpDateTime {
    let a: usize = 0;
    let b: usize = input.len;
    while (a < b && isSpace(input[a])) {
        a += 1;
    }
    while (b > a && isSpace(input[b - 1])) {
        b -= 1;
    }
    const s = input[a..b];
    if (s.len == 29) {
        return parseImfFixdate(s);
    }
    if (s.len == 24) {
        return parseAsctime(s);
    }
    if (s.len >= 30 && s.len <= 33) {
        return parseRfc850(s);
    }
    return invalidDateTime();
}

/// Converts a valid `HttpDateTime` (>= 1970-01-01) into UTC Unix epoch seconds.
/// Returns `-1` if `dt` is invalid.
pub fn toUnixSeconds(dt: HttpDateTime) -> i64 {
    if (!dt.valid || dt.year < 1970 || dt.month < 1 || dt.month > 12) {
        return -1;
    }
    if (dt.day < 1 || dt.day > daysInMonth(dt.year, dt.month)) {
        return -1;
    }
    if (dt.hour > 23 || dt.minute > 59 || dt.second > 59) {
        return -1;
    }
    const days = daysFromCivil(dt.year, dt.month, dt.day);
    const secs = days * 86400 + (dt.hour as u64) * 3600 + (dt.minute as u64) * 60 + (dt.second as u64);
    return secs as i64;
}

/// Converts UTC Unix epoch seconds (`ts >= 0`) into `HttpDateTime` using Hinnant's algorithm.
pub fn fromUnixSeconds(ts: i64) -> HttpDateTime {
    if (ts < 0) {
        return invalidDateTime();
    }
    const uts = ts as u64;
    const days = uts / 86400;
    const rem = uts % 86400;

    const z: u64 = days + 719468;
    const era: u64 = z / 146097;
    const doe: u64 = z - era * 146097;
    const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y: u64 = yoe + era * 400;
    const doy: u64 = doe - (yoe * 365 + yoe / 4 - yoe / 100);
    const mp: u64 = (doy * 5 + 2) / 153;
    const d: u64 = doy - (mp * 153 + 2) / 5 + 1;
    let m: u64 = 0;
    if (mp < 10) {
        m = mp + 3;
    } else {
        m = mp - 9;
    }
    if (m <= 2) {
        y += 1;
    }
    if (y > 9999) {
        return invalidDateTime();
    }

    return HttpDateTime {
        year: y as u16,
        month: m as u8,
        day: d as u8,
        hour: (rem / 3600) as u8,
        minute: ((rem % 3600) / 60) as u8,
        second: (rem % 60) as u8,
        weekday: ((days + 4) % 7) as u8,
        valid: true,
    };
}

fn weekdayName(wd: u8) -> string {
    if (wd == 0) { return "Sun"; }
    if (wd == 1) { return "Mon"; }
    if (wd == 2) { return "Tue"; }
    if (wd == 3) { return "Wed"; }
    if (wd == 4) { return "Thu"; }
    if (wd == 5) { return "Fri"; }
    return "Sat";
}

fn monthName(m: u8) -> string {
    if (m == 1) { return "Jan"; }
    if (m == 2) { return "Feb"; }
    if (m == 3) { return "Mar"; }
    if (m == 4) { return "Apr"; }
    if (m == 5) { return "May"; }
    if (m == 6) { return "Jun"; }
    if (m == 7) { return "Jul"; }
    if (m == 8) { return "Aug"; }
    if (m == 9) { return "Sep"; }
    if (m == 10) { return "Oct"; }
    if (m == 11) { return "Nov"; }
    return "Dec";
}

/// Formats a valid `HttpDateTime` as a strict 29-byte RFC 9110 IMF-fixdate
/// (e.g. `"Sun, 06 Nov 1994 08:49:37 GMT"`). Returns `29`, or `0` if invalid or `out.len < 29`.
pub fn formatImfFixdate(dt: HttpDateTime, out: []u8) -> usize {
    if (!dt.valid || out.len < 29 || dt.year < 1970 || dt.month < 1 || dt.month > 12) {
        return 0;
    }
    const days = daysFromCivil(dt.year, dt.month, dt.day);
    const wd = weekdayName(((days + 4) % 7) as u8);
    const mon = monthName(dt.month);

    out[0] = wd[0];
    out[1] = wd[1];
    out[2] = wd[2];
    out[3] = 44; // ','
    out[4] = 32; // ' '
    out[5] = (48 + (dt.day / 10)) as u8;
    out[6] = (48 + (dt.day % 10)) as u8;
    out[7] = 32;
    out[8] = mon[0];
    out[9] = mon[1];
    out[10] = mon[2];
    out[11] = 32;
    out[12] = (48 + ((dt.year / 1000) % 10)) as u8;
    out[13] = (48 + ((dt.year / 100) % 10)) as u8;
    out[14] = (48 + ((dt.year / 10) % 10)) as u8;
    out[15] = (48 + (dt.year % 10)) as u8;
    out[16] = 32;
    out[17] = (48 + (dt.hour / 10)) as u8;
    out[18] = (48 + (dt.hour % 10)) as u8;
    out[19] = 58; // ':'
    out[20] = (48 + (dt.minute / 10)) as u8;
    out[21] = (48 + (dt.minute % 10)) as u8;
    out[22] = 58; // ':'
    out[23] = (48 + (dt.second / 10)) as u8;
    out[24] = (48 + (dt.second % 10)) as u8;
    out[25] = 32;
    out[26] = 71; // 'G'
    out[27] = 77; // 'M'
    out[28] = 84; // 'T'
    return 29;
}

/// Formats a UTC Unix timestamp directly into a 29-byte IMF-fixdate string.
pub fn formatUnixHttpDate(ts: i64, out: []u8) -> usize {
    const dt = fromUnixSeconds(ts);
    return formatImfFixdate(dt, out);
}

/// Parses an RFC 9110 HTTP-date string directly into UTC Unix seconds, or `-1` on error.
pub fn parseUnixSeconds(input: string) -> i64 {
    const dt = parseHttpDate(input);
    return toUnixSeconds(dt);
}
