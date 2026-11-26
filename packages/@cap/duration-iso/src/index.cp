// @cap/duration-iso — Zero-allocation ISO 8601 Duration Parser, Formatter & Millisecond Converter
// Clean-room MIT implementation in pure CP.

pub struct IsoDuration {
    pub negative: bool,
    pub years: u32,
    pub months: u32,
    pub weeks: u32,
    pub days: u32,
    pub hours: u32,
    pub minutes: u32,
    pub seconds: u32,
    pub millis: u32,
    pub valid: bool,
}

fn invalidDuration() -> IsoDuration {
    return IsoDuration {
        negative: false,
        years: 0,
        months: 0,
        weeks: 0,
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0,
        millis: 0,
        valid: false,
    };
}

/// Parses an ISO 8601 duration string (e.g. `"P1Y2M3DT4H5M6S"`, `"P2W"`, `"-PT15M30.5S"`).
/// Enforces component ordering, forbids duplicate units, rejects empty `"P"` / `"PT"` / `"P1DT"`.
pub fn parseIsoDuration(src: []const u8) -> IsoDuration {
    const n = src.len;
    if (n < 2) {
        return invalidDuration();
    }

    let i: usize = 0;
    let negative = false;
    if (src[i] == 45) {
        // '-'
        negative = true;
        i += 1;
    } else if (src[i] == 43) {
        // '+'
        i += 1;
    }

    if (i >= n || src[i] != 80) {
        // Must start with 'P'
        return invalidDuration();
    }
    i += 1;

    if (i >= n) {
        return invalidDuration();
    }

    let res = IsoDuration {
        negative: negative,
        years: 0,
        months: 0,
        weeks: 0,
        days: 0,
        hours: 0,
        minutes: 0,
        seconds: 0,
        millis: 0,
        valid: false,
    };

    let in_time = false;
    let date_stage: u8 = 0;
    let time_stage: u8 = 0;
    let any_comp = false;
    let time_comp = false;

    while (i < n) {
        if (src[i] == 84) {
            // 'T'
            if (in_time) {
                return invalidDuration();
            }
            in_time = true;
            i += 1;
            continue;
        }

        // Must begin with an ASCII digit
        if (src[i] < 48 || src[i] > 57) {
            return invalidDuration();
        }

        let val: u64 = 0;
        while (i < n && src[i] >= 48 && src[i] <= 57) {
            val = val * 10 + ((src[i] - 48) as u64);
            if (val > 4294967295) {
                return invalidDuration();
            }
            i += 1;
        }

        let has_frac = false;
        let frac_ms: u32 = 0;
        if (i < n && (src[i] == 46 || src[i] == 44)) {
            // '.' or ',' fractional part
            has_frac = true;
            i += 1;
            if (i >= n || src[i] < 48 || src[i] > 57) {
                return invalidDuration();
            }
            let scale: u32 = 100;
            while (i < n && src[i] >= 48 && src[i] <= 57) {
                if (scale > 0) {
                    frac_ms += ((src[i] - 48) as u32) * scale;
                    scale = scale / 10;
                }
                i += 1;
            }
        }

        if (i >= n) {
            return invalidDuration();
        }
        const unit = src[i];
        i += 1;

        if (!in_time) {
            if (has_frac) {
                return invalidDuration();
            }
            if (unit == 89 && date_stage < 1) {
                res.years = val as u32;
                date_stage = 1;
            } else if (unit == 77 && date_stage < 2) {
                res.months = val as u32;
                date_stage = 2;
            } else if (unit == 87 && date_stage < 3) {
                res.weeks = val as u32;
                date_stage = 3;
            } else if (unit == 68 && date_stage < 4) {
                res.days = val as u32;
                date_stage = 4;
            } else {
                return invalidDuration();
            }
            any_comp = true;
        } else {
            if (unit == 72 && time_stage < 1 && !has_frac) {
                res.hours = val as u32;
                time_stage = 1;
            } else if (unit == 77 && time_stage < 2 && !has_frac) {
                res.minutes = val as u32;
                time_stage = 2;
            } else if (unit == 83 && time_stage < 3) {
                res.seconds = val as u32;
                res.millis = frac_ms;
                time_stage = 3;
            } else {
                return invalidDuration();
            }
            any_comp = true;
            time_comp = true;
        }
    }

    if (!any_comp || (in_time && !time_comp)) {
        return invalidDuration();
    }

    res.valid = true;
    return res;
}

fn pushByte(dst: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = b;
    return (pos + 1) as i64;
}

fn pushU32(dst: []u8, pos: usize, val: u32) -> i64 {
    let tmp: [10]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    if (pos + n > dst.len) {
        return -1;
    }
    let k: usize = 0;
    while (k < n) {
        dst[pos + k] = tmp[n - 1 - k];
        k += 1;
    }
    return (pos + n) as i64;
}

fn pushComp(dst: []u8, pos: usize, val: u32, unit: u8) -> i64 {
    const p1 = pushU32(dst, pos, val);
    if (p1 < 0) {
        return -1;
    }
    return pushByte(dst, p1 as usize, unit);
}

/// Formats an `IsoDuration` into canonical shortest ISO 8601 form in `dst`.
/// Zero duration formats as `"PT0S"`. Returns bytes written, or `-1` if `dst` is too small.
pub fn formatIsoDuration(d: IsoDuration, dst: []u8) -> i64 {
    const has_date = d.years > 0 || d.months > 0 || d.weeks > 0 || d.days > 0;
    const has_time = d.hours > 0 || d.minutes > 0 || d.seconds > 0 || d.millis > 0;

    if (!has_date && !has_time) {
        if (dst.len < 4) {
            return -1;
        }
        dst[0] = 80; // 'P'
        dst[1] = 84; // 'T'
        dst[2] = 48; // '0'
        dst[3] = 83; // 'S'
        return 4;
    }

    let out: usize = 0;
    let p: i64 = 0;

    if (d.negative) {
        p = pushByte(dst, out, 45); // '-'
        if (p < 0) { return -1; }
        out = p as usize;
    }

    p = pushByte(dst, out, 80); // 'P'
    if (p < 0) { return -1; }
    out = p as usize;

    if (d.years > 0) {
        p = pushComp(dst, out, d.years, 89);
        if (p < 0) { return -1; }
        out = p as usize;
    }
    if (d.months > 0) {
        p = pushComp(dst, out, d.months, 77);
        if (p < 0) { return -1; }
        out = p as usize;
    }
    if (d.weeks > 0) {
        p = pushComp(dst, out, d.weeks, 87);
        if (p < 0) { return -1; }
        out = p as usize;
    }
    if (d.days > 0) {
        p = pushComp(dst, out, d.days, 68);
        if (p < 0) { return -1; }
        out = p as usize;
    }

    if (has_time) {
        p = pushByte(dst, out, 84); // 'T'
        if (p < 0) { return -1; }
        out = p as usize;

        if (d.hours > 0) {
            p = pushComp(dst, out, d.hours, 72);
            if (p < 0) { return -1; }
            out = p as usize;
        }
        if (d.minutes > 0) {
            p = pushComp(dst, out, d.minutes, 77);
            if (p < 0) { return -1; }
            out = p as usize;
        }
        if (d.seconds > 0 || d.millis > 0) {
            p = pushU32(dst, out, d.seconds);
            if (p < 0) { return -1; }
            out = p as usize;

            if (d.millis > 0) {
                p = pushByte(dst, out, 46); // '.'
                if (p < 0) { return -1; }
                out = p as usize;

                const d1 = (d.millis / 100) % 10;
                const d2 = (d.millis / 10) % 10;
                const d3 = d.millis % 10;

                p = pushByte(dst, out, (48 + d1) as u8);
                if (p < 0) { return -1; }
                out = p as usize;

                if (d2 != 0 || d3 != 0) {
                    p = pushByte(dst, out, (48 + d2) as u8);
                    if (p < 0) { return -1; }
                    out = p as usize;
                }
                if (d3 != 0) {
                    p = pushByte(dst, out, (48 + d3) as u8);
                    if (p < 0) { return -1; }
                    out = p as usize;
                }
            }

            p = pushByte(dst, out, 83); // 'S'
            if (p < 0) { return -1; }
            out = p as usize;
        }
    }

    return out as i64;
}

/// Converts an `IsoDuration` to approximate signed milliseconds
/// using `1Y = 365d`, `1M = 30d`, `1W = 7d`.
pub fn toApproxMillis(d: IsoDuration) -> i64 {
    const total_days: u64 = (d.years as u64) * 365
        + (d.months as u64) * 30
        + (d.weeks as u64) * 7
        + (d.days as u64);
    const total_secs: u64 = total_days * 86400
        + (d.hours as u64) * 3600
        + (d.minutes as u64) * 60
        + (d.seconds as u64);
    const total_ms: i64 = (total_secs * 1000 + (d.millis as u64)) as i64;
    if (d.negative) {
        return -total_ms;
    }
    return total_ms;
}
