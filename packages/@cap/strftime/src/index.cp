// @cap/strftime — Zero-heap POSIX strftime & Civil Calendar Date/Time Formatter
// Clean-room MIT implementation in pure CP (replaces npm `strftime` / `dateformat` [20M/wk]).

pub struct DateTime {
    pub year: i32,
    pub month: u8,     // 1..12
    pub day: u8,       // 1..31
    pub hour: u8,      // 0..23
    pub minute: u8,    // 0..59
    pub second: u8,    // 0..59
    pub weekday: u8,   // 0=Sunday..6=Saturday
    pub yearday: u16,  // 1..366
    pub tz_offset_sec: i32,
}

pub fn isLeapYear(y: i32) -> bool {
    return (@rem(y, 4) == 0 && @rem(y, 100) != 0) || (@rem(y, 400) == 0);
}

fn daysInMonth(y: i32, m: u8) -> u8 {
    if (m == 2) {
        return if (isLeapYear(y)) (29 as u8) else (28 as u8);
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return 30;
    }
    return 31;
}

fn dayOfYear(y: i32, m: u8, d: u8) -> u16 {
    let total: u16 = 0;
    let cur: u8 = 1;
    while (cur < m) {
        total += daysInMonth(y, cur) as u16;
        cur += 1;
    }
    return total + (d as u16);
}

/// Convert Unix epoch seconds (with optional timezone offset in seconds) to broken-down DateTime.
/// Uses Howard Hinnant's civil-from-days algorithm (exact for all proleptic Gregorian dates).
pub fn fromUnixSeconds(unix_sec: i64, tz_offset_sec: i32) -> DateTime {
    const adjusted: i64 = unix_sec + (tz_offset_sec as i64);
    let days: i64 = @divTrunc(adjusted, 86400);
    let rem: i64 = @rem(adjusted, 86400);
    if (rem < 0) {
        rem += 86400;
        days -= 1;
    }

    const hour: u8 = (@divTrunc(rem, 3600)) as u8;
    const minRem: i64 = @rem(rem, 3600);
    const minute: u8 = (@divTrunc(minRem, 60)) as u8;
    const second: u8 = (@rem(minRem, 60)) as u8;

    // 1970-01-01 was Thursday (4)
    let wd: i64 = @rem(days + 4, 7);
    if (wd < 0) { wd += 7; }
    const weekday: u8 = wd as u8;

    // Civil date from days since 1970-01-01
    const z: i64 = days + 719468;
    const era: i64 = @divTrunc(if (z >= 0) z else z - 146096, 146097);
    const doe: i64 = z - era * 146097;
    const yoe: i64 = @divTrunc(doe - @divTrunc(doe, 1460) + @divTrunc(doe, 36524) - @divTrunc(doe, 146096), 365);
    let y: i64 = yoe + era * 400;
    const doy: i64 = doe - (365 * yoe + @divTrunc(yoe, 4) - @divTrunc(yoe, 100));
    const mp: i64 = @divTrunc(5 * doy + 2, 153);
    const d: i64 = doy - @divTrunc(153 * mp + 2, 5) + 1;
    const m: i64 = if (mp < 10) mp + 3 else mp - 9;
    if (m <= 2) { y += 1; }

    const year32: i32 = y as i32;
    const month8: u8 = m as u8;
    const day8: u8 = d as u8;

    return DateTime {
        year: year32,
        month: month8,
        day: day8,
        hour: hour,
        minute: minute,
        second: second,
        weekday: weekday,
        yearday: dayOfYear(year32, month8, day8),
        tz_offset_sec: tz_offset_sec,
    };
}

fn weekdayShort(w: u8) -> string {
    if (w == 0) { return "Sun"; }
    if (w == 1) { return "Mon"; }
    if (w == 2) { return "Tue"; }
    if (w == 3) { return "Wed"; }
    if (w == 4) { return "Thu"; }
    if (w == 5) { return "Fri"; }
    return "Sat";
}

fn weekdayFull(w: u8) -> string {
    if (w == 0) { return "Sunday"; }
    if (w == 1) { return "Monday"; }
    if (w == 2) { return "Tuesday"; }
    if (w == 3) { return "Wednesday"; }
    if (w == 4) { return "Thursday"; }
    if (w == 5) { return "Friday"; }
    return "Saturday";
}

fn monthShort(m: u8) -> string {
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

fn monthFull(m: u8) -> string {
    if (m == 1) { return "January"; }
    if (m == 2) { return "February"; }
    if (m == 3) { return "March"; }
    if (m == 4) { return "April"; }
    if (m == 5) { return "May"; }
    if (m == 6) { return "June"; }
    if (m == 7) { return "July"; }
    if (m == 8) { return "August"; }
    if (m == 9) { return "September"; }
    if (m == 10) { return "October"; }
    if (m == 11) { return "November"; }
    return "December";
}

struct FmtWriter {
    pub out: []u8,
    pub pos: usize,

    pub fn putByte(*_, b: u8) {
        if (_.pos < _.out.len) {
            _.out[_.pos] = b;
            _.pos += 1;
        }
    }

    pub fn putStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len && _.pos < _.out.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
    }

    pub fn putPad2(*_, val: u32, padChar: u8) {
        const tens: u8 = ((val / 10) % 10) as u8;
        const ones: u8 = (val % 10) as u8;
        if (tens == 0 && padChar != '0') {
            _.putByte(padChar);
        } else {
            _.putByte(('0' as u8) + tens);
        }
        _.putByte(('0' as u8) + ones);
    }

    pub fn putPad3(*_, val: u32) {
        _.putByte(('0' as u8) + (((val / 100) % 10) as u8));
        _.putByte(('0' as u8) + (((val / 10) % 10) as u8));
        _.putByte(('0' as u8) + ((val % 10) as u8));
    }

    pub fn putPad4(*_, val: i32) {
        let v: i32 = val;
        if (v < 0) {
            _.putByte('-');
            v = -v;
        }
        const uv: u32 = v as u32;
        _.putByte(('0' as u8) + (((uv / 1000) % 10) as u8));
        _.putByte(('0' as u8) + (((uv / 100) % 10) as u8));
        _.putByte(('0' as u8) + (((uv / 10) % 10) as u8));
        _.putByte(('0' as u8) + ((uv % 10) as u8));
    }
}

/// Format a DateTime using POSIX strftime format specifiers:
/// `%Y`, `%y`, `%m`, `%d`, `%e`, `%H`, `%I`, `%M`, `%S`, `%p`, `%P`,
/// `%a`, `%A`, `%b`, `%B`, `%j`, `%w`, `%u`, `%F`, `%T`, `%R`, `%z`, `%Z`, `%n`, `%t`, `%%`.
pub fn strftime(fmt: string, dt: DateTime, out: []u8) -> string {
    let w: FmtWriter = FmtWriter { out: out, pos: 0 };
    let i: usize = 0;
    while (i < fmt.len) {
        if (fmt[i] == '%' && i + 1 < fmt.len) {
            const spec: u8 = fmt[i + 1];
            i += 2;
            if (spec == '%') {
                w.putByte('%');
            } else if (spec == 'Y') {
                w.putPad4(dt.year);
            } else if (spec == 'y') {
                let yy: i32 = @rem(dt.year, 100);
                if (yy < 0) { yy = -yy; }
                w.putPad2(yy as u32, '0');
            } else if (spec == 'm') {
                w.putPad2(dt.month as u32, '0');
            } else if (spec == 'd') {
                w.putPad2(dt.day as u32, '0');
            } else if (spec == 'e') {
                w.putPad2(dt.day as u32, ' ');
            } else if (spec == 'H') {
                w.putPad2(dt.hour as u32, '0');
            } else if (spec == 'I') {
                let h12: u8 = dt.hour % 12;
                if (h12 == 0) { h12 = 12; }
                w.putPad2(h12 as u32, '0');
            } else if (spec == 'M') {
                w.putPad2(dt.minute as u32, '0');
            } else if (spec == 'S') {
                w.putPad2(dt.second as u32, '0');
            } else if (spec == 'p') {
                w.putStr(if (dt.hour < 12) "AM" else "PM");
            } else if (spec == 'P') {
                w.putStr(if (dt.hour < 12) "am" else "pm");
            } else if (spec == 'a') {
                w.putStr(weekdayShort(dt.weekday));
            } else if (spec == 'A') {
                w.putStr(weekdayFull(dt.weekday));
            } else if (spec == 'b' || spec == 'h') {
                w.putStr(monthShort(dt.month));
            } else if (spec == 'B') {
                w.putStr(monthFull(dt.month));
            } else if (spec == 'j') {
                w.putPad3(dt.yearday as u32);
            } else if (spec == 'w') {
                w.putByte(('0' as u8) + dt.weekday);
            } else if (spec == 'u') {
                const isoW: u8 = if (dt.weekday == 0) (7 as u8) else dt.weekday;
                w.putByte(('0' as u8) + isoW);
            } else if (spec == 'F') {
                w.putPad4(dt.year);
                w.putByte('-');
                w.putPad2(dt.month as u32, '0');
                w.putByte('-');
                w.putPad2(dt.day as u32, '0');
            } else if (spec == 'T') {
                w.putPad2(dt.hour as u32, '0');
                w.putByte(':');
                w.putPad2(dt.minute as u32, '0');
                w.putByte(':');
                w.putPad2(dt.second as u32, '0');
            } else if (spec == 'R') {
                w.putPad2(dt.hour as u32, '0');
                w.putByte(':');
                w.putPad2(dt.minute as u32, '0');
            } else if (spec == 'z') {
                let off: i32 = dt.tz_offset_sec;
                if (off < 0) {
                    w.putByte('-');
                    off = -off;
                } else {
                    w.putByte('+');
                }
                const oh: u32 = (@divTrunc(off, 3600)) as u32;
                const om: u32 = (@divTrunc(@rem(off, 3600), 60)) as u32;
                w.putPad2(oh, '0');
                w.putPad2(om, '0');
            } else if (spec == 'Z') {
                if (dt.tz_offset_sec == 0) {
                    w.putStr("UTC");
                } else {
                    w.putStr("LOCAL");
                }
            } else if (spec == 'n') {
                w.putByte('\n');
            } else if (spec == 't') {
                w.putByte('\t');
            } else {
                w.putByte('%');
                w.putByte(spec);
            }
        } else {
            w.putByte(fmt[i]);
            i += 1;
        }
    }
    return out[0..w.pos];
}

/// Format a Unix timestamp directly using POSIX strftime format string.
pub fn formatUnix(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8) -> string {
    return strftime(fmt, fromUnixSeconds(unix_sec, tz_offset_sec), out);
}

/// Format a Unix timestamp as RFC 3339 / ISO-8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`).
pub fn formatIso8601Utc(unix_sec: i64, out: []u8) -> string {
    return strftime("%FT%TZ", fromUnixSeconds(unix_sec, 0), out);
}
