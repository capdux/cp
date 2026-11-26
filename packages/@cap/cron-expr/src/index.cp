// packages/@cap/cron-expr/src/index.cp
// Zero-allocation 6-field second-granularity Cron expression parser & matcher.
// Clean-room MIT parity with crates.io `cron` v0.12.
// Composes `@cap/cron-parser` for the 5 standard fields (min/hour/dom/month/dow) & presets.

import {
    CronSchedule,
    parseCron,
    matches,
} from "../../cron-parser/src/index.cp";

/// Parsed 6-field Cron schedule (seconds + 5-field standard `CronSchedule`).
pub struct CronExpr6 {
    pub valid: bool,
    pub sec_mask: u64,
    pub base: CronSchedule,

    pub static fn invalid(): CronExpr6 {
        return CronExpr6 {
            valid: false,
            sec_mask: 0 as u64,
            base: CronSchedule.invalid(),
        };
    }
}

fn crxIsSpace(c: u8): bool {
    return c == (32 as u8) || c == (9 as u8) || c == (10 as u8) || c == (13 as u8);
}

fn crxTrim(s: string): string {
    let start: usize = 0;
    while (start < s.len && crxIsSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && crxIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn crxFindByte(s: string, target: u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return s.len;
}

fn crxParseUint(s: string): i32 {
    if (s.len == 0 || s.len > 4) {
        return -1;
    }
    let val: i32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < (48 as u8) || c > (57 as u8)) {
            return -1;
        }
        val = val * 10 + ((c - (48 as u8)) as i32);
        i += 1;
    }
    return val;
}

/// Parse the 1st Cron field (seconds: 0..59) into a 60-bit mask inside `u64`.
/// Supports `*`, `*/step`, `n`, `n/step`, `a-b`, `a-b/step`, and comma-separated lists.
/// Writes `ok_out[0] = true` on success.
fn crxParseSecField(field: string, ok_out: []bool): u64 {
    ok_out[0] = false;
    if (field.len == 0) {
        return 0 as u64;
    }

    let mask: u64 = 0;
    let start: usize = 0;
    let i: usize = 0;

    while (i <= field.len) {
        if (i == field.len || field[i] == (44 as u8)) { // ','
            const item = field[start..i];
            if (item.len == 0) {
                return 0 as u64;
            }

            const slash = crxFindByte(item, 47 as u8); // '/'
            let base = item;
            let step: i32 = 1;
            let has_step = false;
            if (slash < item.len) {
                base = item[0..slash];
                const step_str = item[(slash + 1)..item.len];
                if (crxFindByte(step_str, 47 as u8) < step_str.len) {
                    return 0 as u64;
                }
                step = crxParseUint(step_str);
                if (step <= 0 || step > 59) {
                    return 0 as u64;
                }
                has_step = true;
            }
            if (base.len == 0) {
                return 0 as u64;
            }

            let lo: i32 = 0;
            let hi: i32 = 0;

            if (base.len == 1 && base[0] == (42 as u8)) { // '*'
                lo = 0;
                hi = 59;
            } else {
                const dash = crxFindByte(base, 45 as u8); // '-'
                if (dash < base.len) {
                    const a_str = base[0..dash];
                    const b_str = base[(dash + 1)..base.len];
                    if (crxFindByte(b_str, 45 as u8) < b_str.len) {
                        return 0 as u64;
                    }
                    const a = crxParseUint(a_str);
                    const b = crxParseUint(b_str);
                    if (a < 0 || a > 59 || b < 0 || b > 59 || a > b) {
                        return 0 as u64;
                    }
                    lo = a;
                    hi = b;
                } else {
                    const v = crxParseUint(base);
                    if (v < 0 || v > 59) {
                        return 0 as u64;
                    }
                    lo = v;
                    hi = if (has_step) 59 else v;
                }
            }

            let cur = lo;
            while (cur <= hi) {
                mask = mask | ((1 as u64) << (cur as u6));
                cur += step;
            }

            start = i + 1;
        }
        i += 1;
    }

    if (mask == (0 as u64)) {
        return 0 as u64;
    }
    ok_out[0] = true;
    return mask;
}

/// Parse a 6-field second-granularity Cron expression (`sec min hour dom month dow`),
/// a 5-field standard Cron expression (automatically defaulting seconds to `0`),
/// or a preset alias (`@daily`, `@hourly`, `@weekly`, `@monthly`, `@yearly`, `@annually`, `@midnight`).
pub fn parseCron6(expr: string): CronExpr6 {
    const trimmed = crxTrim(expr);
    if (trimmed.len == 0) {
        return CronExpr6.invalid();
    }

    // Preset aliases (e.g., @daily, @hourly) default to second 0.
    if (trimmed[0] == (64 as u8)) { // '@'
        const base = parseCron(trimmed);
        if (!base.valid) {
            return CronExpr6.invalid();
        }
        return CronExpr6 {
            valid: true,
            sec_mask: 1 as u64,
            base: base,
        };
    }

    // Count whitespace-delimited fields and record the 1st field boundary.
    let fcount: usize = 0;
    let first_start: usize = 0;
    let first_end: usize = 0;
    let rest_start: usize = 0;
    let i: usize = 0;

    while (i < trimmed.len) {
        while (i < trimmed.len && crxIsSpace(trimmed[i])) {
            i += 1;
        }
        if (i >= trimmed.len) {
            break;
        }
        const tok_start = i;
        while (i < trimmed.len && !crxIsSpace(trimmed[i])) {
            i += 1;
        }
        if (fcount == 0) {
            first_start = tok_start;
            first_end = i;
        } else if (fcount == 1) {
            rest_start = tok_start;
        }
        fcount += 1;
        if (fcount > 6) {
            return CronExpr6.invalid();
        }
    }

    if (fcount == 5) {
        const base = parseCron(trimmed);
        if (!base.valid) {
            return CronExpr6.invalid();
        }
        return CronExpr6 {
            valid: true,
            sec_mask: 1 as u64,
            base: base,
        };
    }

    if (fcount == 6) {
        let ok: [1]bool = [false];
        const sec_mask = crxParseSecField(trimmed[first_start..first_end], ok[0..1]);
        if (!ok[0]) {
            return CronExpr6.invalid();
        }
        const base = parseCron(trimmed[rest_start..trimmed.len]);
        if (!base.valid) {
            return CronExpr6.invalid();
        }
        return CronExpr6 {
            valid: true,
            sec_mask: sec_mask,
            base: base,
        };
    }

    return CronExpr6.invalid();
}

/// Check whether `sched` matches the given calendar time including seconds (`sec`: 0..59).
pub fn matches6(
    sched: CronExpr6,
    month: u8,
    dom: u8,
    dow: u8,
    hour: u8,
    minute: u8,
    sec: u8,
): bool {
    if (!sched.valid || sec > (59 as u8)) {
        return false;
    }
    if ((sched.sec_mask & ((1 as u64) << (sec as u6))) == (0 as u64)) {
        return false;
    }
    return matches(sched.base, month, dom, dow, hour, minute);
}

/// Find the next matching second `>= from_sec` (0..59) within the current minute.
/// Returns `null` if `sched` is invalid, `from_sec > 59`, or no second >= `from_sec` matches.
pub fn nextSecInMinute(sched: CronExpr6, from_sec: u8): u8? {
    if (!sched.valid || from_sec > (59 as u8)) {
        return null;
    }
    let s: u8 = from_sec;
    while (s <= (59 as u8)) {
        if ((sched.sec_mask & ((1 as u64) << (s as u6))) != (0 as u64)) {
            return s;
        }
        s += 1;
    }
    return null;
}

/// Return `true` if `expr` is a valid 6-field Cron expression, 5-field Cron expression, or preset alias.
pub fn isValidCron6(expr: string): bool {
    return parseCron6(expr).valid;
}

export {
    CronExpr6,
    parseCron6,
    matches6,
    nextSecInMinute,
    isValidCron6,
};
