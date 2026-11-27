// @cap/postgres-interval — Zero-allocation PostgreSQL INTERVAL parser & formatter in pure CP
// Reference: npm:postgres-interval (https://github.com/bendrucker/postgres-interval, MIT)

import { isSpace, isDigit, isAlpha, toLowerByte, eqIgnoreCase, trim } from "cap:ascii";
import { writeStr, writeU64, writeI64 } from "cap:fmt";

pub struct PgInterval {
    valid: bool,
    years: i32,
    months: i32,
    days: i32,
    hours: i32,
    minutes: i32,
    seconds: i32,
    milliseconds: i32,
}

struct PgivWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,

    fn writeByte(*_, b: u8) {
        if (_.overflow) return;
        if (_.pos >= _.buf.len) {
            _.overflow = true;
            return;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn putStr(*_, s: string) {
        if (_.overflow) return;
        if (_.pos + s.len > _.buf.len) {
            _.overflow = true;
            return;
        }
        _.pos = writeStr(s, _.buf, _.pos);
    }

    fn putU64(*_, val: u64) {
        let tmp: [24]u8 = undefined;
        const len = writeU64(val, tmp[0..24], 0);
        _.putStr(tmp[0..len]);
    }

    fn writeI32(*_, val: i32) {
        let tmp: [24]u8 = undefined;
        const len = writeI64(val as i64, tmp[0..24], 0);
        _.putStr(tmp[0..len]);
    }

    fn writeU64Pad2(*_, val: u64) {
        if (val < 10) {
            _.writeByte('0');
        }
        _.putU64(val);
    }

    fn writeMsTrimmed(*_, ms_abs: u64) {
        if (ms_abs == 0) return;
        _.writeByte('.');
        const d1 = u8((ms_abs / 100) % 10);
        const d2 = u8((ms_abs / 10) % 10);
        const d3 = u8(ms_abs % 10);
        _.writeByte(u8('0') + d1);
        if (d2 != 0 || d3 != 0) {
            _.writeByte(u8('0') + d2);
            if (d3 != 0) {
                _.writeByte(u8('0') + d3);
            }
        }
    }
}

struct PgivIntRes {
    ok: bool,
    val: i64,
    next: usize,
}

struct PgivFracRes {
    ms: i32,
    next: usize,
}

fn pgivInvalid(): PgInterval {
    return PgInterval{
        valid: false,
        years: 0,
        months: 0,
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0,
        milliseconds: 0,
    };
}

fn pgivZero(): PgInterval {
    return PgInterval{
        valid: true,
        years: 0,
        months: 0,
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0,
        milliseconds: 0,
    };
}

fn pgivSkipSpace(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn pgivParseUint(s: string, start: usize): PgivIntRes {
    let i = start;
    if (i >= s.len || !isDigit(s[i])) {
        return PgivIntRes{ ok: false, val: 0, next: start };
    }
    let acc: i64 = 0;
    while (i < s.len && isDigit(s[i])) {
        const d = (s[i] - '0') as i64;
        if (acc > 214748364) {
            return PgivIntRes{ ok: false, val: 0, next: i };
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return PgivIntRes{ ok: true, val: acc, next: i };
}

fn pgivParseFracMs(s: string, start: usize): PgivFracRes {
    let i = start;
    let ms: i32 = 0;
    let scale: i32 = 100;
    while (i < s.len && isDigit(s[i])) {
        const d = (s[i] - '0') as i32;
        if (scale > 0) {
            ms += d * scale;
            scale = @divTrunc(scale, 10);
        }
        i += 1;
    }
    return PgivFracRes{ ms: ms, next: i };
}

fn pgivAbsI32ToU64(v: i32): u64 {
    if (v < 0) {
        const n: i64 = 0 - (v as i64);
        return n as u64;
    }
    return v as u64;
}

fn pgivParseIso(s: string, start: usize, end: usize): PgInterval {
    let iv = pgivZero();
    let i = start;
    let global_sign: i32 = 1;
    if (i < end && (s[i] == '+' || s[i] == '-')) {
        if (s[i] == '-') global_sign = -1;
        i += 1;
    }
    if (i >= end || (s[i] != 'P' && s[i] != 'p')) {
        return pgivInvalid();
    }
    i += 1;
    let in_time = false;
    let parsed_any = false;

    while (i < end) {
        if (s[i] == 'T' || s[i] == 't') {
            if (in_time) return pgivInvalid();
            in_time = true;
            i += 1;
            continue;
        }
        let local_sign: i32 = 1;
        if (s[i] == '+' || s[i] == '-') {
            if (s[i] == '-') local_sign = -1;
            i += 1;
        }
        const num = pgivParseUint(s, i);
        if (!num.ok) return pgivInvalid();
        i = num.next;

        let frac_ms: i32 = 0;
        let has_frac = false;
        if (i < end && (s[i] == '.' || s[i] == ',')) {
            has_frac = true;
            i += 1;
            if (i >= end || !isDigit(s[i])) return pgivInvalid();
            const fr = pgivParseFracMs(s, i);
            frac_ms = fr.ms;
            i = fr.next;
        }
        if (i >= end) return pgivInvalid();
        const unit = toLowerByte(s[i]);
        i += 1;
        const sign = global_sign * local_sign;
        const v32 = (num.val as i32) * sign;
        parsed_any = true;

        if (!in_time) {
            if (has_frac) return pgivInvalid();
            if (unit == 'y') {
                iv.years += v32;
            } else if (unit == 'm') {
                iv.months += v32;
            } else if (unit == 'w') {
                iv.days += v32 * 7;
            } else if (unit == 'd') {
                iv.days += v32;
            } else {
                return pgivInvalid();
            }
        } else {
            if (unit == 'h') {
                if (has_frac) return pgivInvalid();
                iv.hours += v32;
            } else if (unit == 'm') {
                if (has_frac) return pgivInvalid();
                iv.minutes += v32;
            } else if (unit == 's') {
                iv.seconds += v32;
                iv.milliseconds += frac_ms * sign;
            } else {
                return pgivInvalid();
            }
        }
    }

    if (!parsed_any) return pgivInvalid();
    return iv;
}

pub fn parsePgInterval(input: string): PgInterval {
    const trimmed = trim(input);
    if (trimmed.len == 0) return pgivInvalid();
    const start: usize = 0;
    const end: usize = trimmed.len;

    // Detect ISO-8601 duration: P... / +P... / -P...
    if (trimmed[start] == 'P' || trimmed[start] == 'p') {
        return pgivParseIso(trimmed, start, end);
    }
    if ((trimmed[start] == '+' || trimmed[start] == '-') && start + 1 < end &&
        (trimmed[start + 1] == 'P' || trimmed[start + 1] == 'p')) {
        return pgivParseIso(trimmed, start, end);
    }

    let iv = pgivZero();
    let i = start;
    let parsed_any = false;

    // Optional '@' prefix (postgres_verbose)
    if (i < end && trimmed[i] == '@') {
        i += 1;
        i = pgivSkipSpace(trimmed, i);
    }

    while (i < end) {
        while (i < end && (isSpace(trimmed[i]) || trimmed[i] == ',')) {
            i += 1;
        }
        if (i >= end) break;

        // Check trailing "ago"
        if (isAlpha(trimmed[i])) {
            const wstart = i;
            while (i < end && isAlpha(trimmed[i])) {
                i += 1;
            }
            const word = trimmed[wstart..i];
            if (eqIgnoreCase(word, "ago") && parsed_any) {
                const y = iv.years;
                const mo = iv.months;
                const d = iv.days;
                const h = iv.hours;
                const mi = iv.minutes;
                const sec = iv.seconds;
                const ms = iv.milliseconds;
                iv.years = 0 - y;
                iv.months = 0 - mo;
                iv.days = 0 - d;
                iv.hours = 0 - h;
                iv.minutes = 0 - mi;
                iv.seconds = 0 - sec;
                iv.milliseconds = 0 - ms;
                continue;
            }
            return pgivInvalid();
        }

        // Parse optional sign
        let sign: i32 = 1;
        if (trimmed[i] == '+' || trimmed[i] == '-') {
            if (trimmed[i] == '-') sign = -1;
            i += 1;
        }

        const first = pgivParseUint(trimmed, i);
        if (!first.ok) return pgivInvalid();
        i = first.next;

        // Time literal HH:MM[:SS[.fff]]
        if (i < end && trimmed[i] == ':') {
            i += 1;
            const mins = pgivParseUint(trimmed, i);
            if (!mins.ok) return pgivInvalid();
            i = mins.next;

            let secs_val: i64 = 0;
            let frac_ms: i32 = 0;
            if (i < end && trimmed[i] == ':') {
                i += 1;
                const secs = pgivParseUint(trimmed, i);
                if (!secs.ok) return pgivInvalid();
                secs_val = secs.val;
                i = secs.next;

                if (i < end && trimmed[i] == '.') {
                    i += 1;
                    if (i >= end || !isDigit(trimmed[i])) return pgivInvalid();
                    const fr = pgivParseFracMs(trimmed, i);
                    frac_ms = fr.ms;
                    i = fr.next;
                }
            }

            iv.hours += (first.val as i32) * sign;
            iv.minutes += (mins.val as i32) * sign;
            iv.seconds += (secs_val as i32) * sign;
            iv.milliseconds += frac_ms * sign;
            parsed_any = true;
            continue;
        }

        // Optional fractional part before unit (e.g. "1.5 secs")
        let frac_ms: i32 = 0;
        let has_frac = false;
        if (i < end && trimmed[i] == '.') {
            has_frac = true;
            i += 1;
            if (i >= end || !isDigit(trimmed[i])) return pgivInvalid();
            const fr = pgivParseFracMs(trimmed, i);
            frac_ms = fr.ms;
            i = fr.next;
        }

        i = pgivSkipSpace(trimmed, i);
        if (i >= end || !isAlpha(trimmed[i])) {
            return pgivInvalid();
        }
        const ustart = i;
        while (i < end && isAlpha(trimmed[i])) {
            i += 1;
        }
        const unit = trimmed[ustart..i];
        const v32 = (first.val as i32) * sign;

        if (eqIgnoreCase(unit, "year") || eqIgnoreCase(unit, "years") ||
            eqIgnoreCase(unit, "yr") || eqIgnoreCase(unit, "yrs") ||
            eqIgnoreCase(unit, "y")) {
            if (has_frac) return pgivInvalid();
            iv.years += v32;
        } else if (eqIgnoreCase(unit, "mon") || eqIgnoreCase(unit, "mons") ||
            eqIgnoreCase(unit, "month") || eqIgnoreCase(unit, "months")) {
            if (has_frac) return pgivInvalid();
            iv.months += v32;
        } else if (eqIgnoreCase(unit, "week") || eqIgnoreCase(unit, "weeks") ||
            eqIgnoreCase(unit, "w")) {
            if (has_frac) return pgivInvalid();
            iv.days += v32 * 7;
        } else if (eqIgnoreCase(unit, "day") || eqIgnoreCase(unit, "days") ||
            eqIgnoreCase(unit, "d")) {
            if (has_frac) return pgivInvalid();
            iv.days += v32;
        } else if (eqIgnoreCase(unit, "hour") || eqIgnoreCase(unit, "hours") ||
            eqIgnoreCase(unit, "hr") || eqIgnoreCase(unit, "hrs") ||
            eqIgnoreCase(unit, "h")) {
            if (has_frac) return pgivInvalid();
            iv.hours += v32;
        } else if (eqIgnoreCase(unit, "min") || eqIgnoreCase(unit, "mins") ||
            eqIgnoreCase(unit, "minute") || eqIgnoreCase(unit, "minutes") ||
            eqIgnoreCase(unit, "m")) {
            if (has_frac) return pgivInvalid();
            iv.minutes += v32;
        } else if (eqIgnoreCase(unit, "sec") || eqIgnoreCase(unit, "secs") ||
            eqIgnoreCase(unit, "second") || eqIgnoreCase(unit, "seconds") ||
            eqIgnoreCase(unit, "s")) {
            iv.seconds += v32;
            iv.milliseconds += frac_ms * sign;
        } else if (eqIgnoreCase(unit, "ms") || eqIgnoreCase(unit, "msec") ||
            eqIgnoreCase(unit, "msecs") || eqIgnoreCase(unit, "millisecond") ||
            eqIgnoreCase(unit, "milliseconds")) {
            if (has_frac) return pgivInvalid();
            iv.milliseconds += v32;
        } else {
            return pgivInvalid();
        }
        parsed_any = true;
    }

    if (!parsed_any) return pgivInvalid();
    return iv;
}

pub fn toIsoDuration(iv: PgInterval, out: []u8): usize {
    if (!iv.valid) return 0;
    let w = PgivWriter{ buf: out, pos: 0, overflow: false };

    const has_date = iv.years != 0 || iv.months != 0 || iv.days != 0;
    const has_time = iv.hours != 0 || iv.minutes != 0 || iv.seconds != 0 || iv.milliseconds != 0;

    if (!has_date && !has_time) {
        w.putStr("PT0S");
        if (w.overflow) return 0;
        return w.pos;
    }

    w.writeByte('P');
    if (iv.years != 0) {
        w.writeI32(iv.years);
        w.writeByte('Y');
    }
    if (iv.months != 0) {
        w.writeI32(iv.months);
        w.writeByte('M');
    }
    if (iv.days != 0) {
        w.writeI32(iv.days);
        w.writeByte('D');
    }

    if (has_time) {
        w.writeByte('T');
        if (iv.hours != 0) {
            w.writeI32(iv.hours);
            w.writeByte('H');
        }
        if (iv.minutes != 0) {
            w.writeI32(iv.minutes);
            w.writeByte('M');
        }
        if (iv.seconds != 0 || iv.milliseconds != 0) {
            const total_ms: i64 = (iv.seconds as i64) * 1000 + (iv.milliseconds as i64);
            let abs_ms: u64 = 0;
            if (total_ms < 0) {
                w.writeByte('-');
                const neg_ms: i64 = 0 - total_ms;
                abs_ms = neg_ms as u64;
            } else {
                abs_ms = total_ms as u64;
            }
            const sec_part: u64 = abs_ms / 1000;
            const ms_part: u64 = abs_ms % 1000;
            w.putU64(sec_part);
            w.writeMsTrimmed(ms_part);
            w.writeByte('S');
        }
    }

    if (w.overflow) return 0;
    return w.pos;
}

pub fn toPostgresString(iv: PgInterval, out: []u8): usize {
    if (!iv.valid) return 0;
    let w = PgivWriter{ buf: out, pos: 0, overflow: false };

    const has_date = iv.years != 0 || iv.months != 0 || iv.days != 0;
    const has_time = iv.hours != 0 || iv.minutes != 0 || iv.seconds != 0 || iv.milliseconds != 0;

    if (!has_date && !has_time) {
        w.putStr("00:00:00");
        if (w.overflow) return 0;
        return w.pos;
    }

    let need_space = false;
    if (iv.years != 0) {
        w.writeI32(iv.years);
        if (iv.years == 1 || iv.years == -1) {
            w.putStr(" year");
        } else {
            w.putStr(" years");
        }
        need_space = true;
    }
    if (iv.months != 0) {
        if (need_space) w.writeByte(' ');
        w.writeI32(iv.months);
        if (iv.months == 1 || iv.months == -1) {
            w.putStr(" mon");
        } else {
            w.putStr(" mons");
        }
        need_space = true;
    }
    if (iv.days != 0) {
        if (need_space) w.writeByte(' ');
        w.writeI32(iv.days);
        if (iv.days == 1 || iv.days == -1) {
            w.putStr(" day");
        } else {
            w.putStr(" days");
        }
        need_space = true;
    }

    if (has_time) {
        if (need_space) w.writeByte(' ');
        const all_nonneg = iv.hours >= 0 && iv.minutes >= 0 && iv.seconds >= 0 && iv.milliseconds >= 0;
        const all_nonpos = iv.hours <= 0 && iv.minutes <= 0 && iv.seconds <= 0 && iv.milliseconds <= 0;

        if (all_nonneg || all_nonpos) {
            if (all_nonpos) {
                w.writeByte('-');
            }
            w.writeU64Pad2(pgivAbsI32ToU64(iv.hours));
            w.writeByte(':');
            w.writeU64Pad2(pgivAbsI32ToU64(iv.minutes));
            w.writeByte(':');
            w.writeU64Pad2(pgivAbsI32ToU64(iv.seconds));
            const ms_abs = pgivAbsI32ToU64(iv.milliseconds);
            if (ms_abs > 0) {
                w.writeMsTrimmed(ms_abs);
            }
        } else {
            let t_sp = false;
            if (iv.hours != 0) {
                w.writeI32(iv.hours);
                w.putStr(" hours");
                t_sp = true;
            }
            if (iv.minutes != 0) {
                if (t_sp) w.writeByte(' ');
                w.writeI32(iv.minutes);
                w.putStr(" mins");
                t_sp = true;
            }
            if (iv.seconds != 0 || iv.milliseconds != 0) {
                if (t_sp) w.writeByte(' ');
                w.writeI32(iv.seconds);
                if (iv.milliseconds != 0) {
                    w.writeMsTrimmed(pgivAbsI32ToU64(iv.milliseconds));
                }
                w.putStr(" secs");
            }
        }
    }

    if (w.overflow) return 0;
    return w.pos;
}

pub fn totalSecondsApprox(iv: PgInterval): i64 {
    if (!iv.valid) return 0;
    let total: i64 = 0;
    total += (iv.years as i64) * 31536000;
    total += (iv.months as i64) * 2592000;
    total += (iv.days as i64) * 86400;
    total += (iv.hours as i64) * 3600;
    total += (iv.minutes as i64) * 60;
    total += (iv.seconds as i64);
    total += @divTrunc(iv.milliseconds as i64, 1000);
    return total;
}
