// @cap/fill-range — Zero-heap numeric, alphabetic, stepped & zero-padded range expander
// MIT License (clean-room CP implementation)

fn isAsciiAlpha(c: u8) -> bool {
    return (c >= 'a' as u8 && c <= 'z' as u8) || (c >= 'A' as u8 && c <= 'Z' as u8);
}

fn absStepU64(step: i64) -> u64 {
    if (step == 0) {
        return 1;
    }
    if (step > 0) {
        return step as u64;
    }
    return ((-(step + 1)) as u64) + 1;
}

fn absDiffI64(a: i64, b: i64) -> u64 {
    let lo: i64 = a;
    let hi: i64 = b;
    if (a > b) {
        lo = b;
        hi = a;
    }
    if (lo >= 0) {
        return (hi as u64) - (lo as u64);
    }
    if (hi < 0) {
        const lo_mag: u64 = ((-(lo + 1)) as u64) + 1;
        const hi_mag: u64 = ((-(hi + 1)) as u64) + 1;
        return lo_mag - hi_mag;
    }
    const lo_mag: u64 = ((-(lo + 1)) as u64) + 1;
    return (hi as u64) + lo_mag;
}

pub fn countRange(start: i64, end: i64, step: i64) -> usize {
    const st: u64 = absStepU64(step);
    const dist: u64 = absDiffI64(start, end);
    return (dist / st + 1) as usize;
}

pub fn expandNumbers(start: i64, end: i64, step: i64, out: []i64) -> i64 {
    const total: usize = countRange(start, end, step);
    if (total > out.len) {
        return -1;
    }
    const st: u64 = absStepU64(step);
    let cur: i64 = start;
    let i: usize = 0;
    while (i < total) {
        out[i] = cur;
        if (i + 1 < total) {
            if (end >= start) {
                cur = cur + (st as i64);
            } else {
                cur = cur - (st as i64);
            }
        }
        i += 1;
    }
    return total as i64;
}

pub fn expandChars(start: u8, end: u8, step: usize, out: []u8) -> i64 {
    if (!isAsciiAlpha(start) || !isAsciiAlpha(end)) {
        return -1;
    }
    let st: usize = step;
    if (st == 0) {
        st = 1;
    }
    let dist: usize = 0;
    if (end >= start) {
        dist = (end - start) as usize;
    } else {
        dist = (start - end) as usize;
    }
    const total: usize = dist / st + 1;
    if (total > out.len) {
        return -1;
    }
    let cur: usize = start as usize;
    let i: usize = 0;
    while (i < total) {
        out[i] = cur as u8;
        if (i + 1 < total) {
            if (end >= start) {
                cur += st;
            } else {
                cur -= st;
            }
        }
        i += 1;
    }
    return total as i64;
}

struct ParsedInt {
    value: i64,
    pad_width: usize,
    ok: bool,
}

fn parseRangeInt(s: []const u8) -> ParsedInt {
    const fail = ParsedInt { value: 0, pad_width: 0, ok: false };
    if (s.len == 0) {
        return fail;
    }
    let i: usize = 0;
    let is_neg: bool = false;
    if (s[0] == '-' as u8) {
        is_neg = true;
        i = 1;
    } else if (s[0] == '+' as u8) {
        i = 1;
    }
    if (i >= s.len) {
        return fail;
    }
    const digit_start: usize = i;
    const digit_len: usize = s.len - digit_start;

    let pad_width: usize = 0;
    if (digit_len > 1 && s[digit_start] == '0' as u8) {
        pad_width = digit_len;
    }

    const max_pos: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const max_neg: u64 = (0x80000000 as u64) << 32;
    let max_mag: u64 = max_pos;
    if (is_neg) {
        max_mag = max_neg;
    }

    let acc: u64 = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return fail;
        }
        const d: u64 = (c - ('0' as u8)) as u64;
        if (acc > (max_mag - d) / 10) {
            return fail;
        }
        acc = acc * 10 + d;
        i += 1;
    }

    let val: i64 = 0;
    if (is_neg) {
        if (acc == max_neg) {
            val = -(max_pos as i64) - 1;
        } else {
            val = -(acc as i64);
        }
    } else {
        val = acc as i64;
    }
    return ParsedInt { value: val, pad_width: pad_width, ok: true };
}

fn writeSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (src.len > dst.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn writePaddedInt(dst: []u8, start_pos: usize, val: i64, pad_width: usize) -> i64 {
    let pos: usize = start_pos;
    let mag: u64 = 0;
    if (val < 0) {
        if (pos >= dst.len) {
            return -1;
        }
        dst[pos] = '-' as u8;
        pos += 1;
        mag = ((-(val + 1)) as u64) + 1;
    } else {
        mag = val as u64;
    }

    let digits: [24]u8 = undefined;
    let d_len: usize = 0;
    if (mag == 0) {
        digits[0] = '0' as u8;
        d_len = 1;
    } else {
        let cur: u64 = mag;
        while (cur > 0) {
            digits[d_len] = ('0' as u8) + ((cur % 10) as u8);
            d_len += 1;
            cur = cur / 10;
        }
    }

    let zeros: usize = 0;
    if (pad_width > d_len) {
        zeros = pad_width - d_len;
    }
    if (zeros + d_len > dst.len - pos) {
        return -1;
    }

    let z: usize = 0;
    while (z < zeros) {
        dst[pos] = '0' as u8;
        pos += 1;
        z += 1;
    }
    let k: usize = d_len;
    while (k > 0) {
        k -= 1;
        dst[pos] = digits[k];
        pos += 1;
    }
    return pos as i64;
}

pub fn formatRange(
    start: []const u8,
    end: []const u8,
    step: i64,
    sep: []const u8,
    dst: []u8,
) -> i64 {
    if (start.len == 0 || end.len == 0) {
        return -1;
    }

    // Single ASCII letter range: e.g. "a".."e", "F".."A"
    if (start.len == 1 && end.len == 1 && isAsciiAlpha(start[0]) && isAsciiAlpha(end[0])) {
        const s_ch: u8 = start[0];
        const e_ch: u8 = end[0];
        const st_u64: u64 = absStepU64(step);
        let dist: u64 = 0;
        if (e_ch >= s_ch) {
            dist = (e_ch - s_ch) as u64;
        } else {
            dist = (s_ch - e_ch) as u64;
        }
        const total: usize = (dist / st_u64 + 1) as usize;
        const st: usize = st_u64 as usize;

        let pos: usize = 0;
        let cur: usize = s_ch as usize;
        let i: usize = 0;
        while (i < total) {
            if (i > 0) {
                const next_p: i64 = writeSlice(dst, pos, sep);
                if (next_p < 0) {
                    return -1;
                }
                pos = next_p as usize;
            }
            if (pos >= dst.len) {
                return -1;
            }
            dst[pos] = cur as u8;
            pos += 1;
            if (i + 1 < total) {
                if (e_ch >= s_ch) {
                    cur += st;
                } else {
                    cur -= st;
                }
            }
            i += 1;
        }
        return pos as i64;
    }

    // Numeric range (with optional negative sign & zero-padding)
    const ps: ParsedInt = parseRangeInt(start);
    const pe: ParsedInt = parseRangeInt(end);
    if (!ps.ok || !pe.ok) {
        return -1;
    }

    let pad_width: usize = ps.pad_width;
    if (pe.pad_width > pad_width) {
        pad_width = pe.pad_width;
    }

    const total: usize = countRange(ps.value, pe.value, step);
    const st: u64 = absStepU64(step);
    let pos: usize = 0;
    let cur: i64 = ps.value;
    let i: usize = 0;
    while (i < total) {
        if (i > 0) {
            const next_p: i64 = writeSlice(dst, pos, sep);
            if (next_p < 0) {
                return -1;
            }
            pos = next_p as usize;
        }
        const written: i64 = writePaddedInt(dst, pos, cur, pad_width);
        if (written < 0) {
            return -1;
        }
        pos = written as usize;
        if (i + 1 < total) {
            if (pe.value >= ps.value) {
                cur = cur + (st as i64);
            } else {
                cur = cur - (st as i64);
            }
        }
        i += 1;
    }
    return pos as i64;
}
