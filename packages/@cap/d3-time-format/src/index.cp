// @cap/d3-time-format — Zero-heap UTC strftime / strptime formatter & parser
// Clean-room ISC implementation inspired by npm:d3-time-format (d3/d3-time-format).

const D3TF_EPOCH_SHIFT_MS: i64 = 62167219200000; // 719528 * 86400000 (0000-01-01 to 1970-01-01)
const D3TF_MS_PER_DAY: u64 = 86400000;
const D3TF_DAYS_TO_1970_MAR: u64 = 719468;

struct D3tfDateTime {
    year: u64,
    month: u64,   // 1..12
    day: u64,     // 1..31
    hour: u64,    // 0..23
    minute: u64,  // 0..59
    second: u64,  // 0..59
    ms: u64,      // 0..999
    weekday: u64, // 0=Sun..6=Sat
    yearday: u64, // 1..366
}

fn d3tfIsLeapYear(y: u64): bool {
    return ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
}

fn d3tfDaysInMonth(y: u64, m: u64): u64 {
    if (m == 1 || m == 3 || m == 5 || m == 7 || m == 8 || m == 10 || m == 12) {
        return 31;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return 30;
    }
    if (m == 2) {
        return if (d3tfIsLeapYear(y)) 29 else 28;
    }
    return 0;
}

fn d3tfDayOfYear(y: u64, m: u64, d: u64): u64 {
    let acc: u64 = 0;
    let cur: u64 = 1;
    while (cur < m) : (cur += 1) {
        acc += d3tfDaysInMonth(y, cur);
    }
    return acc + d;
}

fn d3tfFromUnixMs(unix_ms: i64): D3tfDateTime {
    const shifted: u64 = u64(unix_ms + D3TF_EPOCH_SHIFT_MS);
    const total_days: u64 = shifted / D3TF_MS_PER_DAY;
    const ms_of_day: u64 = shifted % D3TF_MS_PER_DAY;

    const hour: u64 = ms_of_day / 3600000;
    const rem_h: u64 = ms_of_day % 3600000;
    const minute: u64 = rem_h / 60000;
    const rem_m: u64 = rem_h % 60000;
    const second: u64 = rem_m / 1000;
    const ms: u64 = rem_m % 1000;

    // 1970-01-01 (total_days = 719528, 719528 % 7 == 5) was Thursday (4)
    const weekday: u64 = (total_days + 6) % 7;

    // Howard Hinnant unsigned civil_from_days (shifted to 0000-03-01)
    const z: u64 = total_days - 60;
    const era: u64 = z / 146097;
    const doe: u64 = z - era * 146097;
    const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const y_base: u64 = yoe + era * 400;
    const doy: u64 = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const mp: u64 = (5 * doy + 2) / 153;
    const day: u64 = doy - (153 * mp + 2) / 5 + 1;
    const month: u64 = if (mp < 10) mp + 3 else mp - 9;
    const year: u64 = if (month <= 2) y_base + 1 else y_base;
    const yearday: u64 = d3tfDayOfYear(year, month, day);

    return D3tfDateTime{
        year: year,
        month: month,
        day: day,
        hour: hour,
        minute: minute,
        second: second,
        ms: ms,
        weekday: weekday,
        yearday: yearday,
    };
}

fn d3tfToUnixMs(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64 {
    const y: u64 = if (month <= 2) year - 1 else year;
    const era: u64 = y / 400;
    const yoe: u64 = y - era * 400;
    const mp: u64 = if (month > 2) month - 3 else month + 9;
    const doy: u64 = (153 * mp + 2) / 5 + day - 1;
    const doe: u64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const z: u64 = era * 146097 + doe;
    const days_since_1970: i64 = i64(z) - i64(D3TF_DAYS_TO_1970_MAR);
    const day_ms: i64 = i64(hour * 3600000 + minute * 60000 + second * 1000 + ms);
    return days_since_1970 * 86400000 + day_ms;
}

fn d3tfMonthShort(m: u64): string {
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

fn d3tfMonthFull(m: u64): string {
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

fn d3tfWeekdayShort(w: u64): string {
    if (w == 0) { return "Sun"; }
    if (w == 1) { return "Mon"; }
    if (w == 2) { return "Tue"; }
    if (w == 3) { return "Wed"; }
    if (w == 4) { return "Thu"; }
    if (w == 5) { return "Fri"; }
    return "Sat";
}

fn d3tfWeekdayFull(w: u64): string {
    if (w == 0) { return "Sunday"; }
    if (w == 1) { return "Monday"; }
    if (w == 2) { return "Tuesday"; }
    if (w == 3) { return "Wednesday"; }
    if (w == 4) { return "Thursday"; }
    if (w == 5) { return "Friday"; }
    return "Saturday";
}

fn d3tfWriteStr(out: []u8, pos: usize, s: string): usize {
    let o = pos;
    let i: usize = 0;
    while (i < s.len && o < out.len) : (i += 1) {
        out[o] = s[i];
        o += 1;
    }
    return o;
}

fn d3tfWritePadded(out: []u8, pos: usize, val: u64, width: usize): usize {
    let tmp: [8]u8 = undefined;
    let v = val;
    let i: usize = width;
    while (i > 0) {
        i -= 1;
        tmp[i] = u8('0' + (v % 10));
        v /= 10;
    }
    let o = pos;
    let j: usize = 0;
    while (j < width && o < out.len) : (j += 1) {
        out[o] = tmp[j];
        o += 1;
    }
    return o;
}

/// Format UTC timestamp `unix_ms` using strftime-style `spec` into `out`.
/// Supported directives: %Y, %m, %d, %H, %M, %S, %L, %b, %B, %a, %A, %j, %w, %Z, %%.
pub fn utcFormat(spec: string, unix_ms: i64, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    const dt = d3tfFromUnixMs(unix_ms);
    let o: usize = 0;
    let i: usize = 0;

    while (i < spec.len && o < out.len) {
        if (spec[i] == '%' && i + 1 < spec.len) {
            const code = spec[i + 1];
            i += 2;
            if (code == 'Y') {
                o = d3tfWritePadded(out, o, dt.year, 4);
            } else if (code == 'm') {
                o = d3tfWritePadded(out, o, dt.month, 2);
            } else if (code == 'd') {
                o = d3tfWritePadded(out, o, dt.day, 2);
            } else if (code == 'H') {
                o = d3tfWritePadded(out, o, dt.hour, 2);
            } else if (code == 'M') {
                o = d3tfWritePadded(out, o, dt.minute, 2);
            } else if (code == 'S') {
                o = d3tfWritePadded(out, o, dt.second, 2);
            } else if (code == 'L') {
                o = d3tfWritePadded(out, o, dt.ms, 3);
            } else if (code == 'b') {
                o = d3tfWriteStr(out, o, d3tfMonthShort(dt.month));
            } else if (code == 'B') {
                o = d3tfWriteStr(out, o, d3tfMonthFull(dt.month));
            } else if (code == 'a') {
                o = d3tfWriteStr(out, o, d3tfWeekdayShort(dt.weekday));
            } else if (code == 'A') {
                o = d3tfWriteStr(out, o, d3tfWeekdayFull(dt.weekday));
            } else if (code == 'j') {
                o = d3tfWritePadded(out, o, dt.yearday, 3);
            } else if (code == 'w') {
                o = d3tfWritePadded(out, o, dt.weekday, 1);
            } else if (code == 'Z') {
                o = d3tfWriteStr(out, o, "+0000");
            } else if (code == '%') {
                out[o] = '%';
                o += 1;
            } else {
                out[o] = '%';
                o += 1;
                if (o < out.len) {
                    out[o] = code;
                    o += 1;
                }
            }
        } else {
            out[o] = spec[i];
            o += 1;
            i += 1;
        }
    }

    return o;
}

fn d3tfToLower(ch: u8): u8 {
    if (ch >= 'A' && ch <= 'Z') {
        return ch + 32;
    }
    return ch;
}

fn d3tfMatchWordCI(input: string, pos: usize, word: string): bool {
    if (pos + word.len > input.len) {
        return false;
    }
    let i: usize = 0;
    while (i < word.len) : (i += 1) {
        if (d3tfToLower(input[pos + i]) != d3tfToLower(word[i])) {
            return false;
        }
    }
    return true;
}

struct D3tfUintParse {
    ok: bool,
    val: u64,
    next_pos: usize,
}

fn d3tfParseDigits(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse {
    let p = pos;
    let count: usize = 0;
    let acc: u64 = 0;
    while (p < input.len && count < max_d && input[p] >= '0' && input[p] <= '9') : (p += 1) {
        acc = acc * 10 + u64(input[p] - '0');
        count += 1;
    }
    if (count < min_d) {
        return D3tfUintParse{ ok: false, val: 0, next_pos: pos };
    }
    return D3tfUintParse{ ok: true, val: acc, next_pos: p };
}

/// Parse UTC date string `input` according to `spec`.
/// Returns UTC unix milliseconds, or `null` if input does not match `spec` or is out of range.
pub fn utcParse(spec: string, input: string): i64? {
    let year: u64 = 1970;
    let month: u64 = 1;
    let day: u64 = 1;
    let hour: u64 = 0;
    let minute: u64 = 0;
    let second: u64 = 0;
    let ms: u64 = 0;
    let has_yearday = false;
    let yearday: u64 = 0;
    let tz_offset_ms: i64 = 0;

    let si: usize = 0;
    let ii: usize = 0;

    while (si < spec.len) {
        if (spec[si] == '%' && si + 1 < spec.len) {
            const code = spec[si + 1];
            si += 2;

            if (code == 'Y') {
                const r = d3tfParseDigits(input, ii, 4, 4);
                if (!r.ok) { return null; }
                year = r.val;
                ii = r.next_pos;
            } else if (code == 'm') {
                const r = d3tfParseDigits(input, ii, 1, 2);
                if (!r.ok || r.val < 1 || r.val > 12) { return null; }
                month = r.val;
                ii = r.next_pos;
            } else if (code == 'd') {
                const r = d3tfParseDigits(input, ii, 1, 2);
                if (!r.ok || r.val < 1 || r.val > 31) { return null; }
                day = r.val;
                ii = r.next_pos;
            } else if (code == 'H') {
                const r = d3tfParseDigits(input, ii, 1, 2);
                if (!r.ok || r.val > 23) { return null; }
                hour = r.val;
                ii = r.next_pos;
            } else if (code == 'M') {
                const r = d3tfParseDigits(input, ii, 1, 2);
                if (!r.ok || r.val > 59) { return null; }
                minute = r.val;
                ii = r.next_pos;
            } else if (code == 'S') {
                const r = d3tfParseDigits(input, ii, 1, 2);
                if (!r.ok || r.val > 59) { return null; }
                second = r.val;
                ii = r.next_pos;
            } else if (code == 'L') {
                const r = d3tfParseDigits(input, ii, 1, 3);
                if (!r.ok || r.val > 999) { return null; }
                ms = r.val;
                ii = r.next_pos;
            } else if (code == 'j') {
                const r = d3tfParseDigits(input, ii, 1, 3);
                if (!r.ok || r.val < 1 || r.val > 366) { return null; }
                yearday = r.val;
                has_yearday = true;
                ii = r.next_pos;
            } else if (code == 'w') {
                const r = d3tfParseDigits(input, ii, 1, 1);
                if (!r.ok || r.val > 6) { return null; }
                ii = r.next_pos;
            } else if (code == 'b') {
                let matched = false;
                let m: u64 = 1;
                while (m <= 12) : (m += 1) {
                    const name = d3tfMonthShort(m);
                    if (d3tfMatchWordCI(input, ii, name)) {
                        month = m;
                        ii += name.len;
                        matched = true;
                        break;
                    }
                }
                if (!matched) { return null; }
            } else if (code == 'B') {
                let matched = false;
                let m: u64 = 1;
                while (m <= 12) : (m += 1) {
                    const name = d3tfMonthFull(m);
                    if (d3tfMatchWordCI(input, ii, name)) {
                        month = m;
                        ii += name.len;
                        matched = true;
                        break;
                    }
                }
                if (!matched) { return null; }
            } else if (code == 'a') {
                let matched = false;
                let w: u64 = 0;
                while (w < 7) : (w += 1) {
                    const name = d3tfWeekdayShort(w);
                    if (d3tfMatchWordCI(input, ii, name)) {
                        ii += name.len;
                        matched = true;
                        break;
                    }
                }
                if (!matched) { return null; }
            } else if (code == 'A') {
                let matched = false;
                let w: u64 = 0;
                while (w < 7) : (w += 1) {
                    const name = d3tfWeekdayFull(w);
                    if (d3tfMatchWordCI(input, ii, name)) {
                        ii += name.len;
                        matched = true;
                        break;
                    }
                }
                if (!matched) { return null; }
            } else if (code == 'Z') {
                if (ii < input.len && (input[ii] == 'Z' || input[ii] == 'z')) {
                    ii += 1;
                    tz_offset_ms = 0;
                } else if (ii + 5 <= input.len && (input[ii] == '+' || input[ii] == '-')) {
                    const neg_tz = input[ii] == '-';
                    const rh = d3tfParseDigits(input, ii + 1, 2, 2);
                    const rm = d3tfParseDigits(input, ii + 3, 2, 2);
                    if (!rh.ok || !rm.ok || rh.val > 23 || rm.val > 59) {
                        return null;
                    }
                    const off = i64(rh.val * 3600000 + rm.val * 60000);
                    tz_offset_ms = if (neg_tz) -off else off;
                    ii += 5;
                } else {
                    return null;
                }
            } else if (code == '%') {
                if (ii >= input.len || input[ii] != '%') {
                    return null;
                }
                ii += 1;
            } else {
                return null;
            }
        } else {
            if (ii >= input.len || input[ii] != spec[si]) {
                return null;
            }
            si += 1;
            ii += 1;
        }
    }

    if (ii != input.len) {
        return null;
    }

    if (has_yearday) {
        const max_yd: u64 = if (d3tfIsLeapYear(year)) 366 else 365;
        if (yearday > max_yd) {
            return null;
        }
        let rem = yearday;
        let m: u64 = 1;
        while (m <= 12) : (m += 1) {
            const dim = d3tfDaysInMonth(year, m);
            if (rem <= dim) {
                month = m;
                day = rem;
                break;
            }
            rem -= dim;
        }
    } else {
        const dim = d3tfDaysInMonth(year, month);
        if (day < 1 || day > dim) {
            return null;
        }
    }

    return d3tfToUnixMs(year, month, day, hour, minute, second, ms) - tz_offset_ms;
}
