// @cap/cron-expr — Zero-heap 5-field Unix Cron expression bitmask compiler & O(1) matcher
// MIT License (clean-room CP implementation)

pub struct CronSchedule {
    pub minutes: u64,
    pub hours: u32,
    pub days: u32,
    pub months: u16,
    pub dows: u8,
    pub valid: bool,
}

fn makeInvalid() -> CronSchedule {
    return CronSchedule {
        minutes: 0,
        hours: 0,
        days: 0,
        months: 0,
        dows: 0,
        valid: false,
    };
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn bytesEq(a: string, b: string) -> bool {
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

fn parseU8Strict(s: string) -> i32 {
    if (s.len == 0 || s.len > 3) {
        return -1;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return -1;
        }
        val = val * 10 + ((c - ('0' as u8)) as u32);
        if (val > 255) {
            return -1;
        }
        i += 1;
    }
    return val as i32;
}

fn parseField(
    field: string,
    min_v: u8,
    max_v: u8,
    allow_sun7: bool,
    out_mask: *u64,
) -> bool {
    if (field.len == 0) {
        return false;
    }
    let max_input: u8 = max_v;
    if (allow_sun7) {
        max_input = 7;
    }

    let mask: u64 = 0;
    let pos: usize = 0;

    while (pos <= field.len) {
        let end: usize = pos;
        while (end < field.len && field[end] != ',' as u8) {
            end += 1;
        }
        if (end <= pos) {
            return false;
        }
        const item: string = field[pos..end];

        // Split optional "/step"
        let slash: usize = item.len;
        let k: usize = 0;
        while (k < item.len) {
            if (item[k] == '/' as u8) {
                slash = k;
                break;
            }
            k += 1;
        }

        let step: u32 = 1;
        let has_step: bool = false;
        if (slash < item.len) {
            has_step = true;
            const st_i: i32 = parseU8Strict(item[(slash + 1)..item.len]);
            if (st_i <= 0) {
                return false;
            }
            step = st_i as u32;
        }

        const rng: string = item[0..slash];
        if (rng.len == 0) {
            return false;
        }

        let lo: u32 = 0;
        let hi: u32 = 0;

        if (rng.len == 1 && rng[0] == '*' as u8) {
            lo = min_v as u32;
            hi = max_v as u32;
        } else {
            // Check for '-' range
            let dash: usize = rng.len;
            let d: usize = 0;
            while (d < rng.len) {
                if (rng[d] == '-' as u8) {
                    dash = d;
                    break;
                }
                d += 1;
            }
            if (dash < rng.len) {
                const a_i: i32 = parseU8Strict(rng[0..dash]);
                const b_i: i32 = parseU8Strict(rng[(dash + 1)..rng.len]);
                if (a_i < (min_v as i32) || b_i < (min_v as i32)
                    || a_i > (max_input as i32) || b_i > (max_input as i32)
                    || a_i > b_i) {
                    return false;
                }
                lo = a_i as u32;
                hi = b_i as u32;
            } else {
                const a_i: i32 = parseU8Strict(rng);
                if (a_i < (min_v as i32) || a_i > (max_input as i32)) {
                    return false;
                }
                lo = a_i as u32;
                if (has_step) {
                    hi = max_v as u32;
                } else {
                    hi = lo;
                }
            }
        }

        let cur: u32 = lo;
        while (cur <= hi) {
            let bit: u32 = cur;
            if (allow_sun7 && bit == 7) {
                bit = 0;
            }
            mask = mask | ((1 as u64) << (bit as u6));
            cur += step;
        }

        if (end >= field.len) {
            break;
        }
        pos = end + 1;
        if (pos >= field.len) {
            return false;
        }
    }

    out_mask.* = mask;
    return true;
}

pub fn parseCron(expr: string) -> CronSchedule {
    let s: usize = 0;
    let e: usize = expr.len;
    while (s < e && isWs(expr[s])) {
        s += 1;
    }
    while (e > s && isWs(expr[e - 1])) {
        e -= 1;
    }
    if (s >= e) {
        return makeInvalid();
    }

    let eff: string = expr[s..e];
    if (eff[0] == '@' as u8) {
        if (bytesEq(eff, "@yearly") || bytesEq(eff, "@annually")) {
            eff = "0 0 1 1 *";
        } else if (bytesEq(eff, "@monthly")) {
            eff = "0 0 1 * *";
        } else if (bytesEq(eff, "@weekly")) {
            eff = "0 0 * * 0";
        } else if (bytesEq(eff, "@daily") || bytesEq(eff, "@midnight")) {
            eff = "0 0 * * *";
        } else if (bytesEq(eff, "@hourly")) {
            eff = "0 * * * *";
        } else {
            return makeInvalid();
        }
    }

    // Split into exactly 5 whitespace-separated fields
    let fields: [5]string = undefined;
    let f_count: usize = 0;
    let pos: usize = 0;
    while (pos < eff.len) {
        while (pos < eff.len && isWs(eff[pos])) {
            pos += 1;
        }
        if (pos >= eff.len) {
            break;
        }
        const f_start: usize = pos;
        while (pos < eff.len && !isWs(eff[pos])) {
            pos += 1;
        }
        if (f_count >= 5) {
            return makeInvalid();
        }
        fields[f_count] = eff[f_start..pos];
        f_count += 1;
    }
    if (f_count != 5) {
        return makeInvalid();
    }

    let m_min: u64 = 0;
    let m_hr: u64 = 0;
    let m_day: u64 = 0;
    let m_mon: u64 = 0;
    let m_dow: u64 = 0;

    if (!parseField(fields[0], 0, 59, false, &m_min)) {
        return makeInvalid();
    }
    if (!parseField(fields[1], 0, 23, false, &m_hr)) {
        return makeInvalid();
    }
    if (!parseField(fields[2], 1, 31, false, &m_day)) {
        return makeInvalid();
    }
    if (!parseField(fields[3], 1, 12, false, &m_mon)) {
        return makeInvalid();
    }
    if (!parseField(fields[4], 0, 6, true, &m_dow)) {
        return makeInvalid();
    }

    return CronSchedule {
        minutes: m_min,
        hours: m_hr as u32,
        days: m_day as u32,
        months: m_mon as u16,
        dows: m_dow as u8,
        valid: true,
    };
}

pub fn matches(
    sched: CronSchedule,
    minute: u8,
    hour: u8,
    day: u8,
    month: u8,
    dow: u8,
) -> bool {
    if (!sched.valid) {
        return false;
    }
    if (minute > 59 || hour > 23 || day < 1 || day > 31 || month < 1 || month > 12 || dow > 7) {
        return false;
    }
    let norm_dow: u8 = dow;
    if (norm_dow == 7) {
        norm_dow = 0;
    }

    const ok_min: bool = (sched.minutes & ((1 as u64) << (minute as u6))) != 0;
    const ok_hr: bool = (sched.hours & ((1 as u32) << (hour as u5))) != 0;
    const ok_day: bool = (sched.days & ((1 as u32) << (day as u5))) != 0;
    const ok_mon: bool = (sched.months & ((1 as u16) << (month as u4))) != 0;
    const ok_dow: bool = (sched.dows & ((1 as u8) << (norm_dow as u3))) != 0;
    return ok_min && ok_hr && ok_day && ok_mon && ok_dow;
}

pub fn isValidCron(expr: string) -> bool {
    return parseCron(expr).valid;
}

fn popcount64(v: u64) -> u32 {
    let x: u64 = v;
    let c: u32 = 0;
    while (x != 0) {
        x = x & (x - 1);
        c += 1;
    }
    return c;
}

pub fn countMatchingMinutesPerDay(sched: CronSchedule) -> u32 {
    if (!sched.valid) {
        return 0;
    }
    const mins: u32 = popcount64(sched.minutes);
    const hrs: u32 = popcount64(sched.hours as u64);
    return mins * hrs;
}
