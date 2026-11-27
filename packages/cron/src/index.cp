// @cap/cron-next — Zero-allocation 5-field POSIX Cron bitmask parser & next UTC trigger calculator for CP
// Clean-room MIT implementation inspired by npm `cron-parser` (25M+/wk) and Rust `cron`.

import { isSpace, isDigit, trim } from "cap:ascii";
import { bytesEq } from "cap:bin";

fn bit64(idx: u32) -> u64 {
    let v: u64 = 1;
    let i: u32 = 0;
    while (i < idx) {
        v = v * (2 as u64);
        i += 1 as u32;
    }
    return v;
}

fn bit32(idx: u32) -> u32 {
    return bit64(idx) as u32;
}

fn bit16(idx: u32) -> u16 {
    return bit64(idx) as u16;
}

fn bit8(idx: u32) -> u8 {
    return bit64(idx) as u8;
}

fn parseUint(s: []const u8) -> i32 {
    if (s.len == (0 as usize) || s.len > (4 as usize)) {
        return -1 as i32;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return -1 as i32;
        }
        val = (val * (10 as u32)) + ((c - '0') as u32);
        i += 1;
    }
    return val as i32;
}

struct FieldResult {
    mask: u64,
    is_star: bool,
    ok: bool,
}

fn parseField(field: []const u8, min_val: u32, max_val: u32, is_dow: bool) -> FieldResult {
    if (field.len == (0 as usize)) {
        return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
    }
    let isStar = false;
    if (field.len == (1 as usize) && field[0] == '*') {
        isStar = true;
    }

    let mask: u64 = 0;
    let pos: usize = 0;
    while (pos <= field.len) {
        const segStart = pos;
        while (pos < field.len && field[pos] != ',') {
            pos += 1;
        }
        const seg = field[segStart..pos];
        if (seg.len == (0 as usize)) {
            return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
        }

        // Split optional "/step"
        let slashIdx: usize = seg.len;
        let i: usize = 0;
        while (i < seg.len) {
            if (seg[i] == '/') {
                if (slashIdx != seg.len) {
                    return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                }
                slashIdx = i;
            }
            i += 1;
        }

        let step: u32 = 1;
        if (slashIdx < seg.len) {
            const stepParsed = parseUint(seg[(slashIdx + (1 as usize))..seg.len]);
            if (stepParsed <= (0 as i32)) {
                return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
            }
            step = stepParsed as u32;
        }

        const rangePart = seg[0..slashIdx];
        if (rangePart.len == (0 as usize)) {
            return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
        }

        let rStart: u32 = min_val;
        let rEnd: u32 = max_val;

        if (rangePart.len == (1 as usize) && rangePart[0] == '*') {
            rStart = min_val;
            rEnd = max_val;
        } else {
            let dashIdx: usize = rangePart.len;
            let k: usize = 0;
            while (k < rangePart.len) {
                if (rangePart[k] == '-') {
                    if (dashIdx != rangePart.len) {
                        return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                    }
                    dashIdx = k;
                }
                k += 1;
            }

            if (dashIdx < rangePart.len) {
                const a = parseUint(rangePart[0..dashIdx]);
                const b = parseUint(rangePart[(dashIdx + (1 as usize))..rangePart.len]);
                if (a < (0 as i32) || b < (0 as i32)) {
                    return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                }
                rStart = a as u32;
                rEnd = b as u32;
                if (rStart < min_val || rEnd > max_val || rStart > rEnd) {
                    return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                }
            } else {
                const single = parseUint(rangePart);
                if (single < (0 as i32)) {
                    return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                }
                rStart = single as u32;
                if (rStart < min_val || rStart > max_val) {
                    return FieldResult{ mask: 0 as u64, is_star: false, ok: false };
                }
                if (slashIdx < seg.len) {
                    rEnd = max_val;
                } else {
                    rEnd = rStart;
                }
            }
        }

        let v = rStart;
        while (v <= rEnd) {
            let mapped = v;
            if (is_dow && mapped == (7 as u32)) {
                mapped = 0 as u32;
            }
            mask = mask | bit64(mapped);
            v += step;
        }

        pos += 1;
    }

    return FieldResult{
        mask: mask,
        is_star: isStar,
        ok: mask != (0 as u64),
    };
}

// --- UTC Calendar Conversion (Howard Hinnant unsigned algorithm) ---

struct UtcDateTime {
    year: u32,
    month: u32,
    day: u32,
    hour: u32,
    minute: u32,
    dow: u32,
}

fn isLeapYear(y: u32) -> bool {
    if ((y % (400 as u32)) == (0 as u32)) {
        return true;
    }
    if ((y % (100 as u32)) == (0 as u32)) {
        return false;
    }
    return (y % (4 as u32)) == (0 as u32);
}

fn daysInMonth(y: u32, m: u32) -> u32 {
    if (m == (2 as u32)) {
        if (isLeapYear(y)) {
            return 29 as u32;
        }
        return 28 as u32;
    }
    if (m == (4 as u32) || m == (6 as u32) || m == (9 as u32) || m == (11 as u32)) {
        return 30 as u32;
    }
    return 31 as u32;
}

fn epochSecToUtc(sec: u64) -> UtcDateTime {
    const totalDays = sec / (86400 as u64);
    const rem = sec % (86400 as u64);
    const hour = (rem / (3600 as u64)) as u32;
    const minute = ((rem % (3600 as u64)) / (60 as u64)) as u32;
    const dow = ((totalDays + (4 as u64)) % (7 as u64)) as u32;

    const z = totalDays + (719468 as u64);
    const era = z / (146097 as u64);
    const doe = z - (era * (146097 as u64));
    const yoe = (doe - (doe / (1460 as u64)) + (doe / (36524 as u64)) - (doe / (146096 as u64))) / (365 as u64);
    let y = yoe + (era * (400 as u64));
    const doy = doe - ((365 as u64) * yoe + (yoe / (4 as u64)) - (yoe / (100 as u64)));
    const mp = (((5 as u64) * doy) + (2 as u64)) / (153 as u64);
    const d = (doy - ((((153 as u64) * mp) + (2 as u64)) / (5 as u64)) + (1 as u64)) as u32;
    let m: u32 = 0;
    if (mp < (10 as u64)) {
        m = (mp + (3 as u64)) as u32;
    } else {
        m = (mp - (9 as u64)) as u32;
    }
    if (m <= (2 as u32)) {
        y += 1 as u64;
    }

    return UtcDateTime{
        year: y as u32,
        month: m,
        day: d,
        hour: hour,
        minute: minute,
        dow: dow,
    };
}

fn utcToEpochSec(year: u32, month: u32, day: u32, hour: u32, minute: u32) -> u64 {
    let y = year as u64;
    let mAdj: u64 = 0;
    if (month <= (2 as u32)) {
        y -= 1 as u64;
        mAdj = (month as u64) + (9 as u64);
    } else {
        mAdj = (month as u64) - (3 as u64);
    }
    const era = y / (400 as u64);
    const yoe = y - (era * (400 as u64));
    const doy = ((((153 as u64) * mAdj) + (2 as u64)) / (5 as u64)) + (day as u64) - (1 as u64);
    const doe = (yoe * (365 as u64)) + (yoe / (4 as u64)) - (yoe / (100 as u64)) + doy;
    const totalDays = (era * (146097 as u64)) + doe - (719468 as u64);
    return (totalDays * (86400 as u64)) + ((hour as u64) * (3600 as u64)) + ((minute as u64) * (60 as u64));
}

fn dowForDate(year: u32, month: u32, day: u32) -> u32 {
    const days = utcToEpochSec(year, month, day, 0 as u32, 0 as u32) / (86400 as u64);
    return ((days + (4 as u64)) % (7 as u64)) as u32;
}

pub struct CronSchedule {
    pub min_mask: u64,
    pub hour_mask: u32,
    pub dom_mask: u32,
    pub mon_mask: u16,
    pub dow_mask: u8,
    pub dom_any: bool,
    pub dow_any: bool,
    pub valid: bool,

    pub static fn parse(expr_in: []const u8) -> CronSchedule {
        let expr = trim(expr_in);
        if (bytesEq(expr, "@hourly")) {
            expr = "0 * * * *";
        } else if (bytesEq(expr, "@daily") || bytesEq(expr, "@midnight")) {
            expr = "0 0 * * *";
        } else if (bytesEq(expr, "@weekly")) {
            expr = "0 0 * * 0";
        } else if (bytesEq(expr, "@monthly")) {
            expr = "0 0 1 * *";
        } else if (bytesEq(expr, "@yearly") || bytesEq(expr, "@annually")) {
            expr = "0 0 1 1 *";
        }

        let starts: [5]usize = undefined;
        let ends: [5]usize = undefined;
        let fieldCount: usize = 0;
        let i: usize = 0;
        while (i < expr.len) {
            while (i < expr.len && isSpace(expr[i])) {
                i += 1;
            }
            if (i >= expr.len) {
                break;
            }
            if (fieldCount >= (5 as usize)) {
                return CronSchedule{
                    min_mask: 0 as u64,
                    hour_mask: 0 as u32,
                    dom_mask: 0 as u32,
                    mon_mask: 0 as u16,
                    dow_mask: 0 as u8,
                    dom_any: false,
                    dow_any: false,
                    valid: false,
                };
            }
            starts[fieldCount] = i;
            while (i < expr.len && !isSpace(expr[i])) {
                i += 1;
            }
            ends[fieldCount] = i;
            fieldCount += 1;
        }

        if (fieldCount != (5 as usize)) {
            return CronSchedule{
                min_mask: 0 as u64,
                hour_mask: 0 as u32,
                dom_mask: 0 as u32,
                mon_mask: 0 as u16,
                dow_mask: 0 as u8,
                dom_any: false,
                dow_any: false,
                valid: false,
            };
        }

        const fMin = parseField(expr[starts[0]..ends[0]], 0 as u32, 59 as u32, false);
        const fHour = parseField(expr[starts[1]..ends[1]], 0 as u32, 23 as u32, false);
        const fDom = parseField(expr[starts[2]..ends[2]], 1 as u32, 31 as u32, false);
        const fMon = parseField(expr[starts[3]..ends[3]], 1 as u32, 12 as u32, false);
        const fDow = parseField(expr[starts[4]..ends[4]], 0 as u32, 7 as u32, true);

        if (!fMin.ok || !fHour.ok || !fDom.ok || !fMon.ok || !fDow.ok) {
            return CronSchedule{
                min_mask: 0 as u64,
                hour_mask: 0 as u32,
                dom_mask: 0 as u32,
                mon_mask: 0 as u16,
                dow_mask: 0 as u8,
                dom_any: false,
                dow_any: false,
                valid: false,
            };
        }

        return CronSchedule{
            min_mask: fMin.mask,
            hour_mask: fHour.mask as u32,
            dom_mask: fDom.mask as u32,
            mon_mask: fMon.mask as u16,
            dow_mask: fDow.mask as u8,
            dom_any: fDom.is_star,
            dow_any: fDow.is_star,
            valid: true,
        };
    }

    pub fn isValid() -> bool {
        return _.valid;
    }

    fn matchesDay(day: u32, dow: u32) -> bool {
        const domHit = (_.dom_mask & bit32(day)) != (0 as u32);
        const dowHit = (_.dow_mask & bit8(dow)) != (0 as u8);
        // POSIX rule: union when both dom and dow are restricted, intersection otherwise
        if (!_.dom_any && !_.dow_any) {
            return domHit || dowHit;
        }
        return domHit && dowHit;
    }

    pub fn matchesUtc(epoch_sec: i64) -> bool {
        if (!_.valid || epoch_sec < (0 as i64)) {
            return false;
        }
        const dt = epochSecToUtc(epoch_sec as u64);
        if ((_.mon_mask & bit16(dt.month)) == (0 as u16)) {
            return false;
        }
        if (!_.matchesDay(dt.day, dt.dow)) {
            return false;
        }
        if ((_.hour_mask & bit32(dt.hour)) == (0 as u32)) {
            return false;
        }
        return (_.min_mask & bit64(dt.minute)) != (0 as u64);
    }

    pub fn nextAfter(epoch_sec: i64) -> i64 {
        if (!_.valid) {
            return -1 as i64;
        }
        let baseSec: u64 = 0;
        if (epoch_sec >= (0 as i64)) {
            baseSec = epoch_sec as u64;
        }
        // Strictly after epoch_sec, aligned to minute boundary
        const startSec = ((baseSec / (60 as u64)) + (1 as u64)) * (60 as u64);
        const startDt = epochSecToUtc(startSec);

        let year = startDt.year;
        let month = startDt.month;
        let day = startDt.day;
        let hour = startDt.hour;
        let minute = startDt.minute;
        const maxYear = year + (8 as u32);

        while (year <= maxYear) {
            if ((_.mon_mask & bit16(month)) == (0 as u16)) {
                month += 1 as u32;
                day = 1 as u32;
                hour = 0 as u32;
                minute = 0 as u32;
                if (month > (12 as u32)) {
                    month = 1 as u32;
                    year += 1 as u32;
                }
                continue;
            }

            const dim = daysInMonth(year, month);
            if (day > dim) {
                month += 1 as u32;
                day = 1 as u32;
                hour = 0 as u32;
                minute = 0 as u32;
                if (month > (12 as u32)) {
                    month = 1 as u32;
                    year += 1 as u32;
                }
                continue;
            }

            const dow = dowForDate(year, month, day);
            if (!_.matchesDay(day, dow)) {
                day += 1 as u32;
                hour = 0 as u32;
                minute = 0 as u32;
                continue;
            }

            if ((_.hour_mask & bit32(hour)) == (0 as u32)) {
                hour += 1 as u32;
                minute = 0 as u32;
                if (hour >= (24 as u32)) {
                    hour = 0 as u32;
                    day += 1 as u32;
                }
                continue;
            }

            if ((_.min_mask & bit64(minute)) == (0 as u64)) {
                minute += 1 as u32;
                if (minute >= (60 as u32)) {
                    minute = 0 as u32;
                    hour += 1 as u32;
                    if (hour >= (24 as u32)) {
                        hour = 0 as u32;
                        day += 1 as u32;
                    }
                }
                continue;
            }

            return utcToEpochSec(year, month, day, hour, minute) as i64;
        }

        return -1 as i64;
    }
}

export {
    CronSchedule,
};
