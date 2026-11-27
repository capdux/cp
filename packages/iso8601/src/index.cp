pub struct IsoDate {
    pub year: i32,
    pub month: u8,
    pub day: u8,
    pub ordinal: u16,
    pub week: u8,
    pub weekday: u8,
    pub kind: u8, // 0 = YMD, 1 = Week, 2 = Ordinal
}

pub struct IsoTime {
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
    pub millisecond: u16,
    pub tz_offset_minutes: i16,
    pub has_tz: bool,
}

pub struct IsoDateTime {
    pub date: IsoDate,
    pub time: IsoTime,
}

pub struct IsoDuration {
    pub years: u32,
    pub months: u32,
    pub weeks: u32,
    pub days: u32,
    pub hours: u32,
    pub minutes: u32,
    pub seconds: u32,
    pub milliseconds: u32,
}

fn isoIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn isoParseDigits(s: string, start: usize, count: usize): i64? {
    if (start + count > s.len) {
        return null;
    }
    let val: i64 = 0;
    let i: usize = 0;
    while (i < count) {
        let c = s[start + i];
        if (!isoIsDigit(c)) {
            return null;
        }
        val = val * 10 + ((c - 48) as i64);
        i = i + 1;
    }
    return val;
}

pub fn isLeapYear(year: i32): bool {
    let y: i64 = if (year < 0) { -(year as i64) } else { year as i64 };
    let uy = y as u64;
    return (uy % 4 == 0 && uy % 100 != 0) || (uy % 400 == 0);
}

pub fn daysInMonth(year: i32, month: u8): u8 {
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        return 31;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    if (month == 2) {
        if (isLeapYear(year)) { return 29; }
        return 28;
    }
    return 0;
}

pub fn parseDate(s: string): IsoDate? {
    if (s.len < 7) {
        return null;
    }
    let pos: usize = 0;
    let neg = false;
    if (s[pos] == 45) { // '-'
        neg = true;
        pos = pos + 1;
    } else if (s[pos] == 43) { // '+'
        pos = pos + 1;
    }
    let y_opt = isoParseDigits(s, pos, 4);
    if (y_opt == null) { return null; }
    let year: i32 = (y_opt ?? 0) as i32;
    if (neg) { year = -year; }
    pos = pos + 4;

    if (pos < s.len && s[pos] == 45) {
        pos = pos + 1;
    }

    // Week date: YYYY-Www-D or YYYYWwwD
    if (pos < s.len && (s[pos] == 87 || s[pos] == 119)) { // 'W' / 'w'
        pos = pos + 1;
        let w_opt = isoParseDigits(s, pos, 2);
        if (w_opt == null) { return null; }
        let wk = (w_opt ?? 0) as u8;
        if (wk < 1 || wk > 53) { return null; }
        pos = pos + 2;
        if (pos < s.len && s[pos] == 45) {
            pos = pos + 1;
        }
        let d_opt = isoParseDigits(s, pos, 1);
        if (d_opt == null) { return null; }
        let wd = (d_opt ?? 0) as u8;
        if (wd < 1 || wd > 7) { return null; }
        if (pos + 1 != s.len) { return null; }
        return IsoDate {
            year: year,
            month: 0,
            day: 0,
            ordinal: 0,
            week: wk,
            weekday: wd,
            kind: 1,
        };
    }

    // Ordinal date: YYYY-DDD or YYYYDDD
    let rem = s.len - pos;
    if (rem == 3) {
        let ord_opt = isoParseDigits(s, pos, 3);
        if (ord_opt == null) { return null; }
        let ord = (ord_opt ?? 0) as u16;
        let max_ord: u16 = if (isLeapYear(year)) { 366 } else { 365 };
        if (ord < 1 || ord > max_ord) { return null; }
        return IsoDate {
            year: year,
            month: 0,
            day: 0,
            ordinal: ord,
            week: 0,
            weekday: 0,
            kind: 2,
        };
    }

    // Calendar date: YYYY-MM-DD or YYYYMMDD
    let m_opt = isoParseDigits(s, pos, 2);
    if (m_opt == null) { return null; }
    let month = (m_opt ?? 0) as u8;
    if (month < 1 || month > 12) { return null; }
    pos = pos + 2;
    if (pos < s.len && s[pos] == 45) {
        pos = pos + 1;
    }
    let d_opt = isoParseDigits(s, pos, 2);
    if (d_opt == null) { return null; }
    let day = (d_opt ?? 0) as u8;
    let max_d = daysInMonth(year, month);
    if (day < 1 || day > max_d) { return null; }
    if (pos + 2 != s.len) { return null; }
    return IsoDate {
        year: year,
        month: month,
        day: day,
        ordinal: 0,
        week: 0,
        weekday: 0,
        kind: 0,
    };
}

pub fn parseTime(s: string): IsoTime? {
    if (s.len < 5) {
        return null;
    }
    let pos: usize = 0;
    let h_opt = isoParseDigits(s, pos, 2);
    if (h_opt == null) { return null; }
    let hour = (h_opt ?? 0) as u8;
    if (hour > 24) { return null; }
    pos = pos + 2;

    if (pos < s.len && s[pos] == 58) { pos = pos + 1; }
    let min_opt = isoParseDigits(s, pos, 2);
    if (min_opt == null) { return null; }
    let minute = (min_opt ?? 0) as u8;
    if (minute > 59) { return null; }
    pos = pos + 2;

    let second: u8 = 0;
    if (pos < s.len && (s[pos] == 58 || isoIsDigit(s[pos]))) {
        if (s[pos] == 58) { pos = pos + 1; }
        let sec_opt = isoParseDigits(s, pos, 2);
        if (sec_opt == null) { return null; }
        second = (sec_opt ?? 0) as u8;
        if (second > 60) { return null; } // allow leap second 60
        pos = pos + 2;
    }

    let ms: u16 = 0;
    if (pos < s.len && (s[pos] == 46 || s[pos] == 44)) { // '.' or ','
        pos = pos + 1;
        let frac_digits: usize = 0;
        let mult: u16 = 100;
        while (pos < s.len && isoIsDigit(s[pos])) {
            if (frac_digits < 3) {
                ms = ms + (((s[pos] - 48) as u16) * mult);
                if (mult == 100) { mult = 10; }
                else if (mult == 10) { mult = 1; }
                else { mult = 0; }
            }
            frac_digits = frac_digits + 1;
            pos = pos + 1;
        }
        if (frac_digits == 0) { return null; }
    }

    let has_tz = false;
    let tz_min: i16 = 0;
    if (pos < s.len) {
        let c = s[pos];
        if (c == 90 || c == 122) { // 'Z' or 'z'
            has_tz = true;
            tz_min = 0;
            pos = pos + 1;
        } else if (c == 43 || c == 45) { // '+' or '-'
            has_tz = true;
            let sign: i16 = if (c == 45) { -1 } else { 1 };
            pos = pos + 1;
            let tzh_opt = isoParseDigits(s, pos, 2);
            if (tzh_opt == null) { return null; }
            let tzh = (tzh_opt ?? 0) as i16;
            if (tzh > 23) { return null; }
            pos = pos + 2;
            let tzm: i16 = 0;
            if (pos < s.len) {
                if (s[pos] == 58) { pos = pos + 1; }
                let tzm_opt = isoParseDigits(s, pos, 2);
                if (tzm_opt == null) { return null; }
                tzm = (tzm_opt ?? 0) as i16;
                if (tzm > 59) { return null; }
                pos = pos + 2;
            }
            tz_min = sign * (tzh * 60 + tzm);
        } else {
            return null;
        }
    }

    if (pos != s.len) { return null; }
    return IsoTime {
        hour: hour,
        minute: minute,
        second: second,
        millisecond: ms,
        tz_offset_minutes: tz_min,
        has_tz: has_tz,
    };
}

pub fn parseDateTime(s: string): IsoDateTime? {
    let sep: usize = 0;
    let found = false;
    while (sep < s.len) {
        if (s[sep] == 84 || s[sep] == 116 || s[sep] == 32) { // 'T', 't', ' '
            found = true;
            break;
        }
        sep = sep + 1;
    }
    if (!found || sep == 0 || sep + 1 >= s.len) {
        return null;
    }
    let d_opt = parseDate(s[0..sep]);
    if (d_opt == null) { return null; }
    let t_opt = parseTime(s[(sep + 1)..s.len]);
    if (t_opt == null) { return null; }
    return IsoDateTime {
        date: d_opt ?? IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 0 },
        time: t_opt ?? IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false },
    };
}

pub fn parseDuration(s: string): IsoDuration? {
    if (s.len < 2 || (s[0] != 80 && s[0] != 112)) { // 'P' or 'p'
        return null;
    }
    let dur = IsoDuration {
        years: 0, months: 0, weeks: 0, days: 0,
        hours: 0, minutes: 0, seconds: 0, milliseconds: 0,
    };
    let pos: usize = 1;
    let in_time = false;
    let saw_any = false;
    while (pos < s.len) {
        if (s[pos] == 84 || s[pos] == 116) { // 'T'
            if (in_time) { return null; }
            in_time = true;
            pos = pos + 1;
            continue;
        }
        if (!isoIsDigit(s[pos])) { return null; }
        let num: u32 = 0;
        while (pos < s.len && isoIsDigit(s[pos])) {
            num = num * 10 + ((s[pos] - 48) as u32);
            pos = pos + 1;
        }
        let frac_ms: u32 = 0;
        let has_frac = false;
        if (pos < s.len && (s[pos] == 46 || s[pos] == 44)) {
            has_frac = true;
            pos = pos + 1;
            let mult: u32 = 100;
            let fd: usize = 0;
            while (pos < s.len && isoIsDigit(s[pos])) {
                if (fd < 3) {
                    frac_ms = frac_ms + (((s[pos] - 48) as u32) * mult);
                    if (mult == 100) { mult = 10; }
                    else if (mult == 10) { mult = 1; }
                    else { mult = 0; }
                }
                fd = fd + 1;
                pos = pos + 1;
            }
            if (fd == 0) { return null; }
        }
        if (pos >= s.len) { return null; }
        let unit = s[pos];
        pos = pos + 1;
        saw_any = true;
        if (!in_time) {
            if (has_frac) { return null; }
            if (unit == 89 || unit == 121) { dur.years = num; }       // 'Y'
            else if (unit == 77 || unit == 109) { dur.months = num; } // 'M'
            else if (unit == 87 || unit == 119) { dur.weeks = num; }  // 'W'
            else if (unit == 68 || unit == 100) { dur.days = num; }   // 'D'
            else { return null; }
        } else {
            if (unit == 72 || unit == 104) { // 'H'
                if (has_frac) { return null; }
                dur.hours = num;
            } else if (unit == 77 || unit == 109) { // 'M'
                if (has_frac) { return null; }
                dur.minutes = num;
            } else if (unit == 83 || unit == 115) { // 'S'
                dur.seconds = num;
                dur.milliseconds = frac_ms;
            } else {
                return null;
            }
        }
    }
    if (!saw_any) { return null; }
    return dur;
}

fn isoWrite2(out: []u8, pos: usize, val: u8): usize {
    if (pos + 2 > out.len) { return pos; }
    out[pos] = 48 + (val / 10);
    out[pos + 1] = 48 + (val % 10);
    return pos + 2;
}

fn isoWrite4(out: []u8, pos: usize, val: i32): usize {
    if (pos + 4 > out.len) { return pos; }
    let v: u32 = if (val < 0) { (-val) as u32 } else { val as u32 };
    out[pos] = (48 + ((v / 1000) % 10)) as u8;
    out[pos + 1] = (48 + ((v / 100) % 10)) as u8;
    out[pos + 2] = (48 + ((v / 10) % 10)) as u8;
    out[pos + 3] = (48 + (v % 10)) as u8;
    return pos + 4;
}

pub fn formatDateTime(dt: IsoDateTime, out: []u8): usize {
    if (out.len < 20) { return 0; }
    let p: usize = 0;
    p = isoWrite4(out, p, dt.date.year);
    out[p] = 45; p = p + 1;
    p = isoWrite2(out, p, dt.date.month);
    out[p] = 45; p = p + 1;
    p = isoWrite2(out, p, dt.date.day);
    out[p] = 84; p = p + 1; // 'T'
    p = isoWrite2(out, p, dt.time.hour);
    out[p] = 58; p = p + 1;
    p = isoWrite2(out, p, dt.time.minute);
    out[p] = 58; p = p + 1;
    p = isoWrite2(out, p, dt.time.second);
    if (dt.time.has_tz) {
        if (dt.time.tz_offset_minutes == 0) {
            if (p < out.len) { out[p] = 90; p = p + 1; }
        } else if (p + 6 <= out.len) {
            let off = dt.time.tz_offset_minutes;
            let abs_off: u16 = if (off < 0) { (-off) as u16 } else { off as u16 };
            out[p] = if (off < 0) { 45 } else { 43 };
            p = p + 1;
            p = isoWrite2(out, p, (abs_off / 60) as u8);
            out[p] = 58; p = p + 1;
            p = isoWrite2(out, p, (abs_off % 60) as u8);
        }
    }
    return p;
}
