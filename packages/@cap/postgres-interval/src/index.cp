// @cap/postgres-interval — Zero-allocation PostgreSQL INTERVAL parser & formatter in pure CP
// Reference: npm:postgres-interval (https://github.com/bendrucker/postgres-interval, MIT)

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

    fn writeStr(*_, s: string) {
        if (_.overflow) return;
        if (_.pos + s.len > _.buf.len) {
            _.overflow = true;
            return;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
    }

    fn writeU64(*_, val: u64) {
        let tmp: [24]u8 = undefined;
        let len: usize = 0;
        let v = val;
        if (v == 0) {
            _.writeByte('0');
            return;
        }
        while (v > 0) {
            const digit = u8(v % 10);
            tmp[len] = u8('0') + digit;
            len += 1;
            v /= 10;
        }
        while (len > 0) {
            len -= 1;
            _.writeByte(tmp[len]);
        }
    }

    fn writeI32(*_, val: i32) {
        if (val < 0) {
            _.writeByte('-');
            const neg: i64 = 0 - (val as i64);
            _.writeU64(neg as u64);
        } else {
            _.writeU64(val as u64);
        }
    }

    fn writeU64Pad2(*_, val: u64) {
        if (val < 10) {
            _.writeByte('0');
        }
        _.writeU64(val);
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

fn pgivIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn pgivIsDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn pgivIsAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn pgivToLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn pgivEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (pgivToLower(a[i]) != pgivToLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn pgivSkipSpace(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && pgivIsSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn pgivParseUint(s: string, start: usize): PgivIntRes {
    let i = start;
    if (i >= s.len || !pgivIsDigit(s[i])) {
        return PgivIntRes{ ok: false, val: 0, next: start };
    }
    let acc: i64 = 0;
    while (i < s.len && pgivIsDigit(s[i])) {
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
    while (i < s.len && pgivIsDigit(s[i])) {
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
            if (i >= end || !pgivIsDigit(s[i])) return pgivInvalid();
            const fr = pgivParseFracMs(s, i);
            frac_ms = fr.ms;
            i = fr.next;
        }
        if (i >= end) return pgivInvalid();
        const unit = pgivToLower(s[i]);
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
    let start = pgivSkipSpace(input, 0);
    let end = input.len;
    while (end > start && pgivIsSpace(input[end - 1])) {
        end -= 1;
    }
    if (start >= end) return pgivInvalid();

    // Detect ISO-8601 duration: P... / +P... / -P...
    if (input[start] == 'P' || input[start] == 'p') {
        return pgivParseIso(input, start, end);
    }
    if ((input[start] == '+' || input[start] == '-') && start + 1 < end &&
        (input[start + 1] == 'P' || input[start + 1] == 'p')) {
        return pgivParseIso(input, start, end);
    }

    let iv = pgivZero();
    let i = start;
    let parsed_any = false;

    // Optional '@' prefix (postgres_verbose)
    if (i < end && input[i] == '@') {
        i += 1;
        i = pgivSkipSpace(input, i);
    }

    while (i < end) {
        while (i < end && (pgivIsSpace(input[i]) || input[i] == ',')) {
            i += 1;
        }
        if (i >= end) break;

        // Check trailing "ago"
        if (pgivIsAlpha(input[i])) {
            const wstart = i;
            while (i < end && pgivIsAlpha(input[i])) {
                i += 1;
            }
            const word = input[wstart..i];
            if (pgivEqIgnoreCase(word, "ago") && parsed_any) {
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
        if (input[i] == '+' || input[i] == '-') {
            if (input[i] == '-') sign = -1;
            i += 1;
        }

        const first = pgivParseUint(input, i);
        if (!first.ok) return pgivInvalid();
        i = first.next;

        // Time literal HH:MM[:SS[.fff]]
        if (i < end && input[i] == ':') {
            i += 1;
            const mins = pgivParseUint(input, i);
            if (!mins.ok) return pgivInvalid();
            i = mins.next;

            let secs_val: i64 = 0;
            let frac_ms: i32 = 0;
            if (i < end && input[i] == ':') {
                i += 1;
                const secs = pgivParseUint(input, i);
                if (!secs.ok) return pgivInvalid();
                secs_val = secs.val;
                i = secs.next;

                if (i < end && input[i] == '.') {
                    i += 1;
                    if (i >= end || !pgivIsDigit(input[i])) return pgivInvalid();
                    const fr = pgivParseFracMs(input, i);
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
        if (i < end && input[i] == '.') {
            has_frac = true;
            i += 1;
            if (i >= end || !pgivIsDigit(input[i])) return pgivInvalid();
            const fr = pgivParseFracMs(input, i);
            frac_ms = fr.ms;
            i = fr.next;
        }

        i = pgivSkipSpace(input, i);
        if (i >= end || !pgivIsAlpha(input[i])) {
            return pgivInvalid();
        }
        const ustart = i;
        while (i < end && pgivIsAlpha(input[i])) {
            i += 1;
        }
        const unit = input[ustart..i];
        const v32 = (first.val as i32) * sign;

        if (pgivEqIgnoreCase(unit, "year") || pgivEqIgnoreCase(unit, "years") ||
            pgivEqIgnoreCase(unit, "yr") || pgivEqIgnoreCase(unit, "yrs") ||
            pgivEqIgnoreCase(unit, "y")) {
            if (has_frac) return pgivInvalid();
            iv.years += v32;
        } else if (pgivEqIgnoreCase(unit, "mon") || pgivEqIgnoreCase(unit, "mons") ||
            pgivEqIgnoreCase(unit, "month") || pgivEqIgnoreCase(unit, "months")) {
            if (has_frac) return pgivInvalid();
            iv.months += v32;
        } else if (pgivEqIgnoreCase(unit, "week") || pgivEqIgnoreCase(unit, "weeks") ||
            pgivEqIgnoreCase(unit, "w")) {
            if (has_frac) return pgivInvalid();
            iv.days += v32 * 7;
        } else if (pgivEqIgnoreCase(unit, "day") || pgivEqIgnoreCase(unit, "days") ||
            pgivEqIgnoreCase(unit, "d")) {
            if (has_frac) return pgivInvalid();
            iv.days += v32;
        } else if (pgivEqIgnoreCase(unit, "hour") || pgivEqIgnoreCase(unit, "hours") ||
            pgivEqIgnoreCase(unit, "hr") || pgivEqIgnoreCase(unit, "hrs") ||
            pgivEqIgnoreCase(unit, "h")) {
            if (has_frac) return pgivInvalid();
            iv.hours += v32;
        } else if (pgivEqIgnoreCase(unit, "min") || pgivEqIgnoreCase(unit, "mins") ||
            pgivEqIgnoreCase(unit, "minute") || pgivEqIgnoreCase(unit, "minutes") ||
            pgivEqIgnoreCase(unit, "m")) {
            if (has_frac) return pgivInvalid();
            iv.minutes += v32;
        } else if (pgivEqIgnoreCase(unit, "sec") || pgivEqIgnoreCase(unit, "secs") ||
            pgivEqIgnoreCase(unit, "second") || pgivEqIgnoreCase(unit, "seconds") ||
            pgivEqIgnoreCase(unit, "s")) {
            iv.seconds += v32;
            iv.milliseconds += frac_ms * sign;
        } else if (pgivEqIgnoreCase(unit, "ms") || pgivEqIgnoreCase(unit, "msec") ||
            pgivEqIgnoreCase(unit, "msecs") || pgivEqIgnoreCase(unit, "millisecond") ||
            pgivEqIgnoreCase(unit, "milliseconds")) {
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
        w.writeStr("PT0S");
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
            w.writeU64(sec_part);
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
        w.writeStr("00:00:00");
        if (w.overflow) return 0;
        return w.pos;
    }

    let need_space = false;
    if (iv.years != 0) {
        w.writeI32(iv.years);
        if (iv.years == 1 || iv.years == -1) {
            w.writeStr(" year");
        } else {
            w.writeStr(" years");
        }
        need_space = true;
    }
    if (iv.months != 0) {
        if (need_space) w.writeByte(' ');
        w.writeI32(iv.months);
        if (iv.months == 1 || iv.months == -1) {
            w.writeStr(" mon");
        } else {
            w.writeStr(" mons");
        }
        need_space = true;
    }
    if (iv.days != 0) {
        if (need_space) w.writeByte(' ');
        w.writeI32(iv.days);
        if (iv.days == 1 || iv.days == -1) {
            w.writeStr(" day");
        } else {
            w.writeStr(" days");
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
                w.writeStr(" hours");
                t_sp = true;
            }
            if (iv.minutes != 0) {
                if (t_sp) w.writeByte(' ');
                w.writeI32(iv.minutes);
                w.writeStr(" mins");
                t_sp = true;
            }
            if (iv.seconds != 0 || iv.milliseconds != 0) {
                if (t_sp) w.writeByte(' ');
                w.writeI32(iv.seconds);
                if (iv.milliseconds != 0) {
                    w.writeMsTrimmed(pgivAbsI32ToU64(iv.milliseconds));
                }
                w.writeStr(" secs");
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
