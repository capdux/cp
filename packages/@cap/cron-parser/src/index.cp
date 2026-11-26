// @cap/cron-parser — Zero-heap 5-field standard Cron expression parser & time matcher.
// Clean-room MIT CP implementation modeled on npm `cron-parser` v4.

fn crnIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn crnToLower(c: u8): u8 {
    if (c >= 65 && c <= 90) { return c + 32; }
    return c;
}

fn crnEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (crnToLower(a[i]) != crnToLower(b[i])) { return false; }
        i = i + 1;
    }
    return true;
}

fn crnTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && crnIsSpace(s[start])) {
        start = start + 1;
    }
    let end: usize = s.len;
    while (end > start && crnIsSpace(s[end - 1])) {
        end = end - 1;
    }
    return s[start..end];
}

// Parses a non-empty ASCII unsigned integer; returns -1 on invalid input or overflow.
fn crnParseUint(s: string): i32 {
    if (s.len == 0 || s.len > 4) { return -1; }
    let val: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        if (c < 48 || c > 57) { return -1; }
        val = val * 10 + ((c - 48) as i32);
        i = i + 1;
    }
    return val;
}

fn crnFindByte(s: string, target: u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) { return i; }
        i = i + 1;
    }
    return s.len;
}

fn crnDaysInMonth(month: u8): u8 {
    if (month == 2) { return 28; }
    if (month == 4 || month == 6 || month == 9 || month == 11) { return 30; }
    return 31;
}

// Parses a single cron field into a 64-bit bitmask. Writes `ok_out[0] = true` on success.
fn crnParseField(
    field: string,
    min_val: i32,
    max_val: i32,
    is_dow: bool,
    ok_out: []bool,
): u64 {
    ok_out[0] = false;
    if (field.len == 0) { return 0; }

    let allow_max = if (is_dow) { 7 } else { max_val };
    let mask: u64 = 0;
    let start: usize = 0;
    let i: usize = 0;

    while (i <= field.len) {
        if (i == field.len || field[i] == 44) { // ','
            let item = field[start..i];
            if (item.len == 0) { return 0; }

            // Check optional '/step'
            let slash = crnFindByte(item, 47); // '/'
            let base = item;
            let step: i32 = 1;
            let has_step = false;
            if (slash < item.len) {
                base = item[0..slash];
                let step_str = item[slash + 1..item.len];
                // Reject duplicate '/'
                if (crnFindByte(step_str, 47) < step_str.len) { return 0; }
                step = crnParseUint(step_str);
                if (step <= 0) { return 0; }
                has_step = true;
            }
            if (base.len == 0) { return 0; }

            let lo: i32 = 0;
            let hi: i32 = 0;

            if (base.len == 1 && base[0] == 42) { // '*'
                lo = min_val;
                hi = max_val;
            } else {
                let dash = crnFindByte(base, 45); // '-'
                if (dash < base.len) {
                    let a_str = base[0..dash];
                    let b_str = base[dash + 1..base.len];
                    if (crnFindByte(b_str, 45) < b_str.len) { return 0; }
                    let a = crnParseUint(a_str);
                    let b = crnParseUint(b_str);
                    if (a < min_val || a > allow_max || b < min_val || b > allow_max || a > b) {
                        return 0;
                    }
                    lo = a;
                    hi = b;
                } else {
                    let v = crnParseUint(base);
                    if (v < min_val || v > allow_max) { return 0; }
                    lo = v;
                    if (has_step) {
                        hi = max_val;
                    } else {
                        hi = v;
                    }
                }
            }

            let cur = lo;
            while (cur <= hi) {
                let bit = if (is_dow && cur == 7) { 0 } else { cur };
                mask = mask | ((1 as u64) << (bit as u64));
                cur = cur + step;
            }

            start = i + 1;
        }
        i = i + 1;
    }

    ok_out[0] = true;
    return mask;
}

/// Zero-heap bitmask representation of a 5-field standard Cron schedule.
pub struct CronSchedule {
    pub valid: bool,
    pub min_mask: u64,
    pub hour_mask: u32,
    pub dom_mask: u32,
    pub month_mask: u16,
    pub dow_mask: u8,

    pub static fn invalid(): CronSchedule {
        return CronSchedule {
            valid: false,
            min_mask: 0,
            hour_mask: 0,
            dom_mask: 0,
            month_mask: 0,
            dow_mask: 0,
        };
    }
}

/// Parses a 5-field Cron expression (or `@yearly`/`@annually`/`@monthly`/`@weekly`/`@daily`/`@midnight`/`@hourly`).
pub fn parseCron(expr: string): CronSchedule {
    let trimmed = crnTrim(expr);
    if (trimmed.len == 0) { return CronSchedule.invalid(); }

    let resolved = trimmed;
    if (trimmed[0] == 64) { // '@' preset alias
        if (crnEqIgnoreCase(trimmed, "@yearly") || crnEqIgnoreCase(trimmed, "@annually")) {
            resolved = "0 0 1 1 *";
        } else if (crnEqIgnoreCase(trimmed, "@monthly")) {
            resolved = "0 0 1 * *";
        } else if (crnEqIgnoreCase(trimmed, "@weekly")) {
            resolved = "0 0 * * 0";
        } else if (crnEqIgnoreCase(trimmed, "@daily") || crnEqIgnoreCase(trimmed, "@midnight")) {
            resolved = "0 0 * * *";
        } else if (crnEqIgnoreCase(trimmed, "@hourly")) {
            resolved = "0 * * * *";
        } else {
            return CronSchedule.invalid();
        }
    }

    // Split into exactly 5 whitespace-separated fields
    let fields: [5]string = ["", "", "", "", ""];
    let fcount: usize = 0;
    let i: usize = 0;
    while (i < resolved.len) {
        while (i < resolved.len && crnIsSpace(resolved[i])) {
            i = i + 1;
        }
        if (i >= resolved.len) { break; }
        let start = i;
        while (i < resolved.len && !crnIsSpace(resolved[i])) {
            i = i + 1;
        }
        if (fcount >= 5) {
            return CronSchedule.invalid();
        }
        fields[fcount] = resolved[start..i];
        fcount = fcount + 1;
    }

    if (fcount != 5) {
        return CronSchedule.invalid();
    }

    let ok: [1]bool = [false];
    let m_mask = crnParseField(fields[0], 0, 59, false, ok[0..1]);
    if (!ok[0]) { return CronSchedule.invalid(); }

    let h_mask = crnParseField(fields[1], 0, 23, false, ok[0..1]);
    if (!ok[0]) { return CronSchedule.invalid(); }

    let d_mask = crnParseField(fields[2], 1, 31, false, ok[0..1]);
    if (!ok[0]) { return CronSchedule.invalid(); }

    let mo_mask = crnParseField(fields[3], 1, 12, false, ok[0..1]);
    if (!ok[0]) { return CronSchedule.invalid(); }

    let w_mask = crnParseField(fields[4], 0, 6, true, ok[0..1]);
    if (!ok[0]) { return CronSchedule.invalid(); }

    return CronSchedule {
        valid: true,
        min_mask: m_mask,
        hour_mask: h_mask as u32,
        dom_mask: d_mask as u32,
        month_mask: mo_mask as u16,
        dow_mask: w_mask as u8,
    };
}

/// Checks whether a parsed `CronSchedule` matches the given calendar time
/// (`month`: 1..12, `dom`: 1..31, `dow`: 0..6 or 7=Sun, `hour`: 0..23, `minute`: 0..59).
pub fn matches(
    sched: CronSchedule,
    month: u8,
    dom: u8,
    dow: u8,
    hour: u8,
    minute: u8,
): bool {
    if (!sched.valid) { return false; }
    if (month < 1 || month > 12) { return false; }
    if (dom < 1 || dom > 31) { return false; }
    if (dow > 7) { return false; }
    if (hour > 23) { return false; }
    if (minute > 59) { return false; }

    let norm_dow = if (dow == 7) { 0 as u8 } else { dow };

    if ((sched.min_mask & ((1 as u64) << (minute as u64))) == 0) { return false; }
    if ((sched.hour_mask & ((1 as u32) << (hour as u32))) == 0) { return false; }
    if ((sched.dom_mask & ((1 as u32) << (dom as u32))) == 0) { return false; }
    if ((sched.month_mask & ((1 as u16) << (month as u16))) == 0) { return false; }
    if ((sched.dow_mask & ((1 as u8) << norm_dow)) == 0) { return false; }
    return true;
}

/// Searches forward up to 24 hours (1440 minutes) from the given time for the next
/// trigger minute offset (1..=1440). Returns `null` if no match occurs within 24 hours.
pub fn nextMinuteOffset(
    sched: CronSchedule,
    month: u8,
    dom: u8,
    dow: u8,
    hour: u8,
    minute: u8,
): u32? {
    if (!sched.valid) { return null; }
    if (month < 1 || month > 12 || dom < 1 || dom > 31 || dow > 7 || hour > 23 || minute > 59) {
        return null;
    }

    let cur_month = month;
    let cur_dom = dom;
    let cur_dow: u8 = if (dow == 7) { 0 } else { dow };
    let cur_hour = hour;
    let cur_min = minute;

    let offset: u32 = 1;
    while (offset <= 1440) {
        cur_min = cur_min + 1;
        if (cur_min >= 60) {
            cur_min = 0;
            cur_hour = cur_hour + 1;
            if (cur_hour >= 24) {
                cur_hour = 0;
                cur_dow = (cur_dow + 1) % 7;
                cur_dom = cur_dom + 1;
                if (cur_dom > crnDaysInMonth(cur_month)) {
                    cur_dom = 1;
                    cur_month = cur_month + 1;
                    if (cur_month > 12) {
                        cur_month = 1;
                    }
                }
            }
        }
        if (matches(sched, cur_month, cur_dom, cur_dow, cur_hour, cur_min)) {
            return offset;
        }
        offset = offset + 1;
    }

    return null;
}

/// Returns `true` if `expr` is a valid 5-field Cron expression or preset alias.
pub fn isValidCron(expr: string): bool {
    return parseCron(expr).valid;
}
