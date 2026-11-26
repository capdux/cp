// @cap/d3-time — Zero-allocation UTC calendar time intervals in pure CP
// Reference: npm:d3-time (https://github.com/d3/d3-time, ISC)

struct D3TmYmd {
    year: i64,
    month: i64,
    day: i64,
}

fn d3tmIsLeapYear(y: i64): bool {
    if (@mod(y, 4) != 0) return false;
    if (@mod(y, 100) != 0) return true;
    return @mod(y, 400) == 0;
}

fn d3tmDaysInMonth(y: i64, m: i64): i64 {
    if (m == 1 || m == 3 || m == 5 || m == 7 || m == 8 || m == 10 || m == 12) return 31;
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    if (d3tmIsLeapYear(y)) return 29;
    return 28;
}

// Howard Hinnant unsigned-safe civil_from_days (1970-01-01 == 0)
fn d3tmCivilFromDays(z_in: i64): D3TmYmd {
    const z: i64 = z_in + 719468;
    const era: i64 = @divFloor(z, 146097);
    const doe: u64 = (z - era * 146097) as u64;
    const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const y: i64 = (yoe as i64) + era * 400;
    const doy: u64 = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const mp: u64 = (5 * doy + 2) / 153;
    const d: u64 = doy - (153 * mp + 2) / 5 + 1;
    let m: u64 = 0;
    if (mp < 10) {
        m = mp + 3;
    } else {
        m = mp - 9;
    }
    let yr: i64 = y;
    if (m <= 2) {
        yr += 1;
    }
    return D3TmYmd{ year: yr, month: m as i64, day: d as i64 };
}

// Howard Hinnant unsigned-safe days_from_civil (1970-01-01 == 0)
fn d3tmDaysFromCivil(y_in: i64, m: i64, d: i64): i64 {
    let y: i64 = y_in;
    if (m <= 2) {
        y -= 1;
    }
    const era: i64 = @divFloor(y, 400);
    const yoe: u64 = (y - era * 400) as u64;
    let m_adj: u64 = 0;
    if (m > 2) {
        m_adj = (m - 3) as u64;
    } else {
        m_adj = (m + 9) as u64;
    }
    const doy: u64 = (153 * m_adj + 2) / 5 + (d as u64) - 1;
    const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (doe as i64) - 719468;
}

pub fn utcFromYmd(year: i64, month: i64, day: i64): i64 {
    return d3tmDaysFromCivil(year, month, day) * 86400000;
}

// --- Second ---

pub fn timeSecondFloor(ms: i64): i64 {
    return @divFloor(ms, 1000) * 1000;
}

pub fn timeSecondCeil(ms: i64): i64 {
    const f = timeSecondFloor(ms);
    if (f == ms) return f;
    return f + 1000;
}

pub fn timeSecondOffset(ms: i64, step: i64): i64 {
    return ms + step * 1000;
}

pub fn timeSecondCount(start_ms: i64, end_ms: i64): i64 {
    return @divFloor(timeSecondFloor(end_ms) - timeSecondFloor(start_ms), 1000);
}

// --- Minute ---

pub fn timeMinuteFloor(ms: i64): i64 {
    return @divFloor(ms, 60000) * 60000;
}

pub fn timeMinuteOffset(ms: i64, step: i64): i64 {
    return ms + step * 60000;
}

// --- Hour ---

pub fn timeHourFloor(ms: i64): i64 {
    return @divFloor(ms, 3600000) * 3600000;
}

pub fn timeHourOffset(ms: i64, step: i64): i64 {
    return ms + step * 3600000;
}

// --- Day ---

pub fn timeDayFloor(ms: i64): i64 {
    return @divFloor(ms, 86400000) * 86400000;
}

pub fn timeDayCeil(ms: i64): i64 {
    const f = timeDayFloor(ms);
    if (f == ms) return f;
    return f + 86400000;
}

pub fn timeDayOffset(ms: i64, step: i64): i64 {
    return ms + step * 86400000;
}

pub fn timeDayCount(start_ms: i64, end_ms: i64): i64 {
    return @divFloor(timeDayFloor(end_ms) - timeDayFloor(start_ms), 86400000);
}

// --- Week (Sunday-based, matching d3.utcWeek) ---

pub fn timeWeekFloor(ms: i64): i64 {
    const days = @divFloor(ms, 86400000);
    // 1970-01-01 is Thursday (4)
    const dow = @mod(days + 4, 7);
    return (days - dow) * 86400000;
}

pub fn timeWeekOffset(ms: i64, step: i64): i64 {
    return ms + step * 604800000;
}

// --- Month ---

pub fn timeMonthFloor(ms: i64): i64 {
    const days = @divFloor(ms, 86400000);
    const ymd = d3tmCivilFromDays(days);
    return d3tmDaysFromCivil(ymd.year, ymd.month, 1) * 86400000;
}

pub fn timeMonthOffset(ms: i64, step: i64): i64 {
    const days = @divFloor(ms, 86400000);
    const rem = @mod(ms, 86400000);
    const ymd = d3tmCivilFromDays(days);
    const total_m = ymd.year * 12 + (ymd.month - 1) + step;
    const ny = @divFloor(total_m, 12);
    const nm = @mod(total_m, 12) + 1;
    const max_d = d3tmDaysInMonth(ny, nm);
    let nd = ymd.day;
    if (nd > max_d) nd = max_d;
    return d3tmDaysFromCivil(ny, nm, nd) * 86400000 + rem;
}

// --- Year ---

pub fn timeYearFloor(ms: i64): i64 {
    const days = @divFloor(ms, 86400000);
    const ymd = d3tmCivilFromDays(days);
    return d3tmDaysFromCivil(ymd.year, 1, 1) * 86400000;
}

pub fn timeYearOffset(ms: i64, step: i64): i64 {
    const days = @divFloor(ms, 86400000);
    const rem = @mod(ms, 86400000);
    const ymd = d3tmCivilFromDays(days);
    const ny = ymd.year + step;
    const max_d = d3tmDaysInMonth(ny, ymd.month);
    let nd = ymd.day;
    if (nd > max_d) nd = max_d;
    return d3tmDaysFromCivil(ny, ymd.month, nd) * 86400000 + rem;
}
