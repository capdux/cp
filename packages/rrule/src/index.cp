// @cap/rrule-parse — Zero-allocation RFC 5545 iCalendar RRULE parser & formatter for CP
// Clean-room MIT implementation inspired by npm `rrule` v2.
// Composes `@cap/unicase` (`eqAscii`) for case-insensitive RFC 5545 key & enum matching.

import { eqAscii } from "../../unicase/src/index.cp";
import { trim, isDigit, findByte } from "cap:ascii";
import { parseU32, BufWriter } from "cap:fmt";

pub const FREQ_YEARLY: u8 = 0;
pub const FREQ_MONTHLY: u8 = 1;
pub const FREQ_WEEKLY: u8 = 2;
pub const FREQ_DAILY: u8 = 3;
pub const FREQ_HOURLY: u8 = 4;
pub const FREQ_MINUTELY: u8 = 5;
pub const FREQ_SECONDLY: u8 = 6;

pub const DAY_SU: u8 = 1;
pub const DAY_MO: u8 = 2;
pub const DAY_TU: u8 = 4;
pub const DAY_WE: u8 = 8;
pub const DAY_TH: u8 = 16;
pub const DAY_FR: u8 = 32;
pub const DAY_SA: u8 = 64;

/// Parsed RFC 5545 recurrence rule (zero heap allocation).
pub struct RRule {
    pub valid: bool,
    pub freq: u8,
    pub interval: u32,
    pub count: u32,
    pub has_count: bool,
    pub until: string,
    pub byday_mask: u8,
    pub bymonth_mask: u16,
    pub wkst: u8,
}

fn rrlInvalid(): RRule {
    return RRule {
        valid: false,
        freq: FREQ_DAILY,
        interval: 1,
        count: 0,
        has_count: false,
        until: "",
        byday_mask: 0,
        bymonth_mask: 0,
        wkst: DAY_MO,
    };
}

fn rrlParsePosU32(s: string): u32 {
    if (s.len == 0 || s[0] == 43) {
        return 0;
    }
    return parseU32(s) ?? 0;
}

fn rrlParseFreq(v: string): u8 {
    if (eqAscii(v, "YEARLY")) { return FREQ_YEARLY; }
    if (eqAscii(v, "MONTHLY")) { return FREQ_MONTHLY; }
    if (eqAscii(v, "WEEKLY")) { return FREQ_WEEKLY; }
    if (eqAscii(v, "DAILY")) { return FREQ_DAILY; }
    if (eqAscii(v, "HOURLY")) { return FREQ_HOURLY; }
    if (eqAscii(v, "MINUTELY")) { return FREQ_MINUTELY; }
    if (eqAscii(v, "SECONDLY")) { return FREQ_SECONDLY; }
    return 255;
}

fn rrlParseDayCode(code: string): u8 {
    if (eqAscii(code, "SU")) { return DAY_SU; }
    if (eqAscii(code, "MO")) { return DAY_MO; }
    if (eqAscii(code, "TU")) { return DAY_TU; }
    if (eqAscii(code, "WE")) { return DAY_WE; }
    if (eqAscii(code, "TH")) { return DAY_TH; }
    if (eqAscii(code, "FR")) { return DAY_FR; }
    if (eqAscii(code, "SA")) { return DAY_SA; }
    return 0;
}

/// Parse a single BYDAY item: optional signed ordinal prefix (e.g. `1MO`, `-1FR`, `+2TU`) + 2-letter day.
fn rrlParseByDayItem(item: string): u8 {
    if (item.len < 2) {
        return 0;
    }
    let i: usize = 0;
    if (item.len > 2) {
        if (item[0] == 43 || item[0] == 45) { // '+' or '-'
            i = 1;
            if (item.len - i <= 2) {
                return 0;
            }
        }
        while (i < item.len - 2) {
            if (!isDigit(item[i])) {
                return 0;
            }
            i = i + 1;
        }
    }
    return rrlParseDayCode(item[(item.len - 2)..item.len]);
}

fn rrlParse2Digits(s: string, pos: usize): u32 {
    if (pos + 2 > s.len || !isDigit(s[pos]) || !isDigit(s[pos + 1])) {
        return 999;
    }
    return ((s[pos] - 48) as u32) * 10 + ((s[pos + 1] - 48) as u32);
}

/// Validate RFC 5545 UNTIL value: `YYYYMMDD` or `YYYYMMDDTHHMMSS[Z]`.
fn rrlIsValidUntil(v: string): bool {
    if (v.len != 8 && v.len != 15 && v.len != 16) {
        return false;
    }
    // Check YYYY
    let i: usize = 0;
    while (i < 4) {
        if (!isDigit(v[i])) {
            return false;
        }
        i = i + 1;
    }
    const mm = rrlParse2Digits(v, 4);
    const dd = rrlParse2Digits(v, 6);
    if (mm < 1 || mm > 12 || dd < 1 || dd > 31) {
        return false;
    }
    if (v.len == 8) {
        return true;
    }
    // 'T' or 't'
    if (v[8] != 84 && v[8] != 116) {
        return false;
    }
    const hh = rrlParse2Digits(v, 9);
    const mi = rrlParse2Digits(v, 11);
    const ss = rrlParse2Digits(v, 13);
    if (hh > 23 || mi > 59 || ss > 60) {
        return false;
    }
    if (v.len == 16 && v[15] != 90 && v[15] != 122) { // 'Z' or 'z'
        return false;
    }
    return true;
}

/// Parse an RFC 5545 RRULE string (with optional `RRULE:` prefix).
/// Strictly rejects missing `FREQ`, simultaneous `COUNT` and `UNTIL`, duplicate `FREQ`, or invalid values.
pub fn parseRRule(input: string): RRule {
    let s = trim(input);
    if (s.len == 0) {
        return rrlInvalid();
    }

    if (s.len >= 6 && eqAscii(s[0..6], "RRULE:")) {
        s = trim(s[6..s.len]);
        if (s.len == 0) {
            return rrlInvalid();
        }
    }

    let has_freq = false;
    let freq: u8 = FREQ_DAILY;
    let interval: u32 = 1;
    let count: u32 = 0;
    let has_count = false;
    let until: string = "";
    let byday_mask: u8 = 0;
    let bymonth_mask: u16 = 0;
    let wkst: u8 = DAY_MO;

    let pos: usize = 0;
    while (pos < s.len) {
        const semi = findByte(s, 59 as u8, pos) ?? s.len; // ';'
        const part = trim(s[pos..semi]);
        if (part.len == 0) {
            return rrlInvalid();
        }

        const eq = findByte(part, 61 as u8, 0) ?? part.len; // '='
        if (eq == 0 || eq >= part.len - 1) {
            return rrlInvalid();
        }
        const key = trim(part[0..eq]);
        const val = trim(part[(eq + 1)..part.len]);
        if (key.len == 0 || val.len == 0) {
            return rrlInvalid();
        }

        if (eqAscii(key, "FREQ")) {
            if (has_freq) {
                return rrlInvalid();
            }
            const f = rrlParseFreq(val);
            if (f == 255) {
                return rrlInvalid();
            }
            freq = f;
            has_freq = true;
        } else if (eqAscii(key, "INTERVAL")) {
            const iv = rrlParsePosU32(val);
            if (iv == 0) {
                return rrlInvalid();
            }
            interval = iv;
        } else if (eqAscii(key, "COUNT")) {
            const c = rrlParsePosU32(val);
            if (c == 0) {
                return rrlInvalid();
            }
            count = c;
            has_count = true;
        } else if (eqAscii(key, "UNTIL")) {
            if (!rrlIsValidUntil(val)) {
                return rrlInvalid();
            }
            until = val;
        } else if (eqAscii(key, "BYDAY")) {
            let dpos: usize = 0;
            while (dpos <= val.len) {
                const comma = findByte(val, 44 as u8, dpos) ?? val.len; // ','
                const item = trim(val[dpos..comma]);
                const mask = rrlParseByDayItem(item);
                if (mask == 0) {
                    return rrlInvalid();
                }
                byday_mask = byday_mask | mask;
                if (comma >= val.len) {
                    break;
                }
                dpos = comma + 1;
            }
        } else if (eqAscii(key, "BYMONTH")) {
            let mpos: usize = 0;
            while (mpos <= val.len) {
                const comma = findByte(val, 44 as u8, mpos) ?? val.len; // ','
                const item = trim(val[mpos..comma]);
                const m = rrlParsePosU32(item);
                if (m < 1 || m > 12) {
                    return rrlInvalid();
                }
                bymonth_mask = bymonth_mask | ((1 as u16) << ((m - 1) as u4));
                if (comma >= val.len) {
                    break;
                }
                mpos = comma + 1;
            }
        } else if (eqAscii(key, "WKST")) {
            const w = rrlParseDayCode(val);
            if (w == 0) {
                return rrlInvalid();
            }
            wkst = w;
        } else {
            return rrlInvalid();
        }

        if (semi >= s.len) {
            break;
        }
        pos = semi + 1;
    }

    if (!has_freq) {
        return rrlInvalid();
    }
    // RFC 5545 Section 3.3.10: UNTIL and COUNT MUST NOT both occur in the same recur
    if (has_count && until.len > 0) {
        return rrlInvalid();
    }

    return RRule {
        valid: true,
        freq: freq,
        interval: interval,
        count: count,
        has_count: has_count,
        until: until,
        byday_mask: byday_mask,
        bymonth_mask: bymonth_mask,
        wkst: wkst,
    };
}

fn rrlFreqName(freq: u8): string {
    if (freq == FREQ_YEARLY) { return "YEARLY"; }
    if (freq == FREQ_MONTHLY) { return "MONTHLY"; }
    if (freq == FREQ_WEEKLY) { return "WEEKLY"; }
    if (freq == FREQ_DAILY) { return "DAILY"; }
    if (freq == FREQ_HOURLY) { return "HOURLY"; }
    if (freq == FREQ_MINUTELY) { return "MINUTELY"; }
    if (freq == FREQ_SECONDLY) { return "SECONDLY"; }
    return "DAILY";
}

fn rrlDayName(mask: u8): string {
    if (mask == DAY_SU) { return "SU"; }
    if (mask == DAY_MO) { return "MO"; }
    if (mask == DAY_TU) { return "TU"; }
    if (mask == DAY_WE) { return "WE"; }
    if (mask == DAY_TH) { return "TH"; }
    if (mask == DAY_FR) { return "FR"; }
    if (mask == DAY_SA) { return "SA"; }
    return "MO";
}

/// Format a valid `RRule` into canonical RFC 5545 `FREQ=...;...` notation in `out`.
/// Returns total bytes written (0 if `r.valid` is false or `out` is empty).
pub fn formatRRule(r: RRule, out: []u8): usize {
    if (!r.valid || out.len == 0) {
        return 0;
    }

    let w = BufWriter.new(out);
    w.writeStr("FREQ=");
    w.writeStr(rrlFreqName(r.freq));

    if (r.interval > 1) {
        w.writeStr(";INTERVAL=");
        w.writeU64(r.interval as u64);
    }

    if (r.has_count) {
        w.writeStr(";COUNT=");
        w.writeU64(r.count as u64);
    }

    if (r.until.len > 0) {
        w.writeStr(";UNTIL=");
        w.writeStr(r.until);
    }

    if (r.byday_mask != 0) {
        w.writeStr(";BYDAY=");
        let first = true;
        let bit: u8 = 0;
        while (bit < 7) {
            const mask: u8 = (1 as u8) << (bit as u3);
            if ((r.byday_mask & mask) != 0) {
                if (!first) {
                    w.writeByte(44 as u8); // ','
                }
                w.writeStr(rrlDayName(mask));
                first = false;
            }
            bit = bit + 1;
        }
    }

    if (r.bymonth_mask != 0) {
        w.writeStr(";BYMONTH=");
        let first = true;
        let m: u32 = 1;
        while (m <= 12) {
            const mask: u16 = (1 as u16) << ((m - 1) as u4);
            if ((r.bymonth_mask & mask) != 0) {
                if (!first) {
                    w.writeByte(44 as u8); // ','
                }
                w.writeU64(m as u64);
                first = false;
            }
            m = m + 1;
        }
    }

    if (r.wkst != 0 && r.wkst != DAY_MO) {
        w.writeStr(";WKST=");
        w.writeStr(rrlDayName(r.wkst));
    }

    return w.written();
}

/// Check whether `dow` (`0=Sun..6=Sat`) matches the rule's `BYDAY` constraint.
/// If `byday_mask == 0`, all weekdays match (for a valid rule).
pub fn matchesWeekday(r: RRule, dow: u8): bool {
    if (!r.valid || dow > 6) {
        return false;
    }
    if (r.byday_mask == 0) {
        return true;
    }
    const mask: u8 = (1 as u8) << (dow as u3);
    return (r.byday_mask & mask) != 0;
}

/// Return `true` if `input` is a valid RFC 5545 RRULE string.
pub fn isValidRRule(input: string): bool {
    return parseRRule(input).valid;
}

export {
    FREQ_YEARLY,
    FREQ_MONTHLY,
    FREQ_WEEKLY,
    FREQ_DAILY,
    FREQ_HOURLY,
    FREQ_MINUTELY,
    FREQ_SECONDLY,
    DAY_SU,
    DAY_MO,
    DAY_TU,
    DAY_WE,
    DAY_TH,
    DAY_FR,
    DAY_SA,
    RRule,
    parseRRule,
    formatRRule,
    matchesWeekday,
    isValidRRule,
};
