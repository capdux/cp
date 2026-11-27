// @cap/postgres-date — Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ parser in pure CP
// Clean-room MIT implementation of npm `postgres-date` (#505, 112M/wk).

pub enum PgDateKind {
    Invalid,
    Finite,
    PosInfinity,
    NegInfinity,
}

pub struct PgDateTime {
    pub kind: PgDateKind,
    pub year: i32,
    pub month: u8,
    pub day: u8,
    pub hour: u8,
    pub minute: u8,
    pub second: u8,
    pub microsecond: u32,
    pub tz_offset_sec: i32, // seconds east of UTC (e.g. +08:00 -> +28800)
    pub is_bc: bool,

    pub fn isValid(): bool {
        return _.kind != .Invalid;
    }
}

fn pgdStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn pgdIsDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

fn pgdParseDigits(s: string): i32? {
    if (s.len == 0) { return null; }
    let acc: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!pgdIsDigit(s[i])) { return null; }
        acc = acc * 10 + ((s[i] - 48) as i32);
        i += 1;
    }
    return acc;
}

fn pgdIsLeap(y: i32): bool {
    const yu: u32 = if (y < 0) (0 - y) as u32 else y as u32;
    return (yu % 4 == 0 && yu % 100 != 0) || (yu % 400 == 0);
}

fn pgdDaysInMonth(y: i32, m: u8): u8 {
    if (m == 2) {
        if (pgdIsLeap(y)) { return 29; }
        return 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) { return 30; }
    return 31;
}

/// Parses a PostgreSQL `DATE`, `TIMESTAMP`, `TIMESTAMPTZ`, `infinity`, or `-infinity` string.
pub fn parsePgDate(input: string): PgDateTime {
    const inv = PgDateTime {
        kind: .Invalid, year: 0, month: 0, day: 0,
        hour: 0, minute: 0, second: 0, microsecond: 0,
        tz_offset_sec: 0, is_bc: false,
    };
    if (pgdStrEq(input, "infinity")) {
        return PgDateTime {
            kind: .PosInfinity, year: 0, month: 0, day: 0,
            hour: 0, minute: 0, second: 0, microsecond: 0,
            tz_offset_sec: 0, is_bc: false,
        };
    }
    if (pgdStrEq(input, "-infinity")) {
        return PgDateTime {
            kind: .NegInfinity, year: 0, month: 0, day: 0,
            hour: 0, minute: 0, second: 0, microsecond: 0,
            tz_offset_sec: 0, is_bc: false,
        };
    }

    let s = input;
    let is_bc = false;
    if (s.len >= 3 && s[s.len - 3] == 32 && s[s.len - 2] == 66 && s[s.len - 1] == 67) { // " BC"
        is_bc = true;
        s = s[0..(s.len - 3)];
    }

    // Parse YYYY-MM-DD
    let dash1: usize = 0;
    while (dash1 < s.len && s[dash1] != 45) { dash1 += 1; }
    if (dash1 < 4 || dash1 + 6 > s.len) { return inv; }
    if (s[dash1 + 3] != 45) { return inv; }

    const y_opt = pgdParseDigits(s[0..dash1]);
    const m_opt = pgdParseDigits(s[(dash1 + 1)..(dash1 + 3)]);
    const d_opt = pgdParseDigits(s[(dash1 + 4)..(dash1 + 6)]);
    if (y_opt == null || m_opt == null || d_opt == null) { return inv; }
    const year = y_opt ?? 0;
    const month = (m_opt ?? 0) as u8;
    const day = (d_opt ?? 0) as u8;
    if (month < 1 || month > 12 || day < 1 || day > pgdDaysInMonth(year, month)) {
        return inv;
    }

    let pos = dash1 + 6;
    let hour: u8 = 0;
    let minute: u8 = 0;
    let second: u8 = 0;
    let micro: u32 = 0;
    let tz_sec: i32 = 0;

    if (pos < s.len) {
        if (s[pos] != 32 && s[pos] != 84 && s[pos] != 116) { return inv; } // ' ', 'T', 't'
        pos += 1;
        if (pos + 8 > s.len || s[pos + 2] != 58 || s[pos + 5] != 58) { return inv; }
        const h_opt = pgdParseDigits(s[pos..(pos + 2)]);
        const mi_opt = pgdParseDigits(s[(pos + 3)..(pos + 5)]);
        const sc_opt = pgdParseDigits(s[(pos + 6)..(pos + 8)]);
        if (h_opt == null || mi_opt == null || sc_opt == null) { return inv; }
        hour = (h_opt ?? 0) as u8;
        minute = (mi_opt ?? 0) as u8;
        second = (sc_opt ?? 0) as u8;
        if (hour > 23 || minute > 59 || second > 60) { return inv; }
        pos += 8;

        // Optional fractional seconds (.123456)
        if (pos < s.len && s[pos] == 46) {
            pos += 1;
            let scale: u32 = 100000;
            let digits_read: usize = 0;
            while (pos < s.len && pgdIsDigit(s[pos])) {
                if (scale > 0) {
                    micro += ((s[pos] - 48) as u32) * scale;
                    scale = scale / 10;
                }
                digits_read += 1;
                pos += 1;
            }
            if (digits_read == 0) { return inv; }
        }

        // Optional timezone: Z / +HH / +HH:MM / +HHMM / +HH:MM:SS
        if (pos < s.len) {
            const tb = s[pos];
            if (tb == 90 || tb == 122) { // 'Z' or 'z'
                pos += 1;
                if (pos != s.len) { return inv; }
            } else if (tb == 43 || tb == 45) { // '+' or '-'
                let sign: i32 = 1;
                if (tb == 45) { sign = -1; }
                pos += 1;
                if (pos + 2 > s.len) { return inv; }
                const tzh_opt = pgdParseDigits(s[pos..(pos + 2)]);
                if (tzh_opt == null) { return inv; }
                const tzh = tzh_opt ?? 0;
                pos += 2;
                let tzm: i32 = 0;
                let tzs: i32 = 0;
                if (pos < s.len) {
                    if (s[pos] == 58) { pos += 1; }
                    if (pos + 2 > s.len) { return inv; }
                    const tzm_opt = pgdParseDigits(s[pos..(pos + 2)]);
                    if (tzm_opt == null) { return inv; }
                    tzm = tzm_opt ?? 0;
                    pos += 2;
                    if (pos < s.len && s[pos] == 58) {
                        pos += 1;
                        if (pos + 2 > s.len) { return inv; }
                        const tzs_opt = pgdParseDigits(s[pos..(pos + 2)]);
                        if (tzs_opt == null) { return inv; }
                        tzs = tzs_opt ?? 0;
                        pos += 2;
                    }
                }
                if (pos != s.len || tzh > 23 || tzm > 59 || tzs > 59) { return inv; }
                tz_sec = sign * (tzh * 3600 + tzm * 60 + tzs);
            } else {
                return inv;
            }
        }
    }

    return PgDateTime {
        kind: .Finite,
        year: year,
        month: month,
        day: day,
        hour: hour,
        minute: minute,
        second: second,
        microsecond: micro,
        tz_offset_sec: tz_sec,
        is_bc: is_bc,
    };
}

/// Converts a finite AD `PgDateTime` to UTC Unix epoch milliseconds (Howard Hinnant civil algorithm).
pub fn toUnixMs(dt: PgDateTime): i64? {
    if (dt.kind != .Finite || dt.is_bc) {
        return null;
    }
    let y: i64 = dt.year as i64;
    let m: i64 = dt.month as i64;
    const d: i64 = dt.day as i64;
    if (m <= 2) {
        y -= 1;
    }
    let era: i64 = 0;
    if (y >= 0) {
        era = ((y as u64) / 400) as i64;
    } else {
        era = -((((399 - y) as u64) / 400) as i64);
    }
    const yoe: u64 = (y - era * 400) as u64;
    let m_adj: u64 = 0;
    if (m > 2) { m_adj = (m - 3) as u64; } else { m_adj = (m + 9) as u64; }
    const doy: u64 = (153 * m_adj + 2) / 5 + (d as u64) - 1;
    const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const days: i64 = era * 146097 + (doe as i64) - 719468;
    const secs = days * 86400
        + (dt.hour as i64) * 3600
        + (dt.minute as i64) * 60
        + (dt.second as i64)
        - (dt.tz_offset_sec as i64);
    return secs * 1000 + ((dt.microsecond / 1000) as i64);
}

export {
    PgDateKind,
    PgDateTime,
    parsePgDate,
    toUnixMs,
};
