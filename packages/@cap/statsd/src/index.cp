// @cap/statsd — Zero-heap StatsD & DogStatsD UDP metric codec
// MIT License (clean-room CP implementation)

pub const STATSD_INVALID: u8 = 0;
pub const STATSD_COUNTER: u8 = 1;
pub const STATSD_TIMING: u8 = 2;
pub const STATSD_GAUGE: u8 = 3;
pub const STATSD_SET: u8 = 4;

pub struct StatsdMetric {
    pub name_start: usize,
    pub name_len: usize,
    pub value: i64,
    pub kind: u8,
    pub sample_rate_milli: u16,
    pub tags_start: usize,
    pub tags_len: usize,
    pub valid: bool,
}

fn bytesEq(a: []const u8, b: []const u8) -> bool {
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

fn isValidMetricName(name: []const u8) -> bool {
    if (name.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) {
        const c: u8 = name[i];
        if (c == ':' as u8 || c == '|' as u8 || c == '\n' as u8 || c == '\r' as u8) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidTags(tags: []const u8) -> bool {
    let i: usize = 0;
    while (i < tags.len) {
        const c: u8 = tags[i];
        if (c == '|' as u8 || c == '\n' as u8 || c == '\r' as u8) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendByte(dst: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = b;
    return (pos + 1) as i64;
}

fn appendSlice(dst: []u8, pos: usize, s: []const u8) -> i64 {
    if (s.len > dst.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        dst[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

fn appendI64(dst: []u8, pos: usize, v: i64) -> i64 {
    let p: i64 = pos as i64;
    let mag: u64 = 0;
    if (v < 0) {
        p = appendByte(dst, p as usize, '-' as u8);
        if (p < 0) {
            return -1;
        }
        mag = ((-(v + 1)) as u64) + 1;
    } else {
        mag = v as u64;
    }

    let digits: [24]u8 = undefined;
    let count: usize = 0;
    if (mag == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u64 = mag;
        while (cur > 0) {
            digits[count] = ('0' as u8) + ((cur % 10) as u8);
            count += 1;
            cur = cur / 10;
        }
    }

    let k: usize = count;
    while (k > 0) {
        k -= 1;
        p = appendByte(dst, p as usize, digits[k]);
        if (p < 0) {
            return -1;
        }
    }
    return p;
}

pub fn formatMetric(
    name: []const u8,
    value: i64,
    kind: u8,
    sample_rate_milli: u16,
    tags: []const u8,
    dst: []u8,
) -> i64 {
    if (!isValidMetricName(name) || !isValidTags(tags)) {
        return -1;
    }
    if (sample_rate_milli == 0 || sample_rate_milli > 1000) {
        return -1;
    }

    let type_str: []const u8 = "";
    if (kind == STATSD_COUNTER) {
        type_str = "c";
    } else if (kind == STATSD_TIMING) {
        type_str = "ms";
    } else if (kind == STATSD_GAUGE) {
        type_str = "g";
    } else if (kind == STATSD_SET) {
        type_str = "s";
    } else {
        return -1;
    }

    let p: i64 = appendSlice(dst, 0, name);
    if (p < 0) {
        return -1;
    }
    p = appendByte(dst, p as usize, ':' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendI64(dst, p as usize, value);
    if (p < 0) {
        return -1;
    }
    p = appendByte(dst, p as usize, '|' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, type_str);
    if (p < 0) {
        return -1;
    }

    if (sample_rate_milli < 1000) {
        p = appendSlice(dst, p as usize, "|@0.");
        if (p < 0) {
            return -1;
        }
        const d1: u8 = ((sample_rate_milli / 100) % 10) as u8;
        const d2: u8 = ((sample_rate_milli / 10) % 10) as u8;
        const d3: u8 = (sample_rate_milli % 10) as u8;
        p = appendByte(dst, p as usize, ('0' as u8) + d1);
        if (p < 0) {
            return -1;
        }
        if (d2 != 0 || d3 != 0) {
            p = appendByte(dst, p as usize, ('0' as u8) + d2);
            if (p < 0) {
                return -1;
            }
        }
        if (d3 != 0) {
            p = appendByte(dst, p as usize, ('0' as u8) + d3);
            if (p < 0) {
                return -1;
            }
        }
    }

    if (tags.len > 0) {
        p = appendSlice(dst, p as usize, "|#");
        if (p < 0) {
            return -1;
        }
        p = appendSlice(dst, p as usize, tags);
        if (p < 0) {
            return -1;
        }
    }

    return p;
}

struct IntParseResult {
    value: i64,
    ok: bool,
}

fn parseI64Slice(s: []const u8) -> IntParseResult {
    const fail = IntParseResult { value: 0, ok: false };
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

    if (is_neg) {
        if (acc == max_neg) {
            return IntParseResult { value: -(max_pos as i64) - 1, ok: true };
        }
        return IntParseResult { value: -(acc as i64), ok: true };
    }
    return IntParseResult { value: acc as i64, ok: true };
}

fn parseSampleRateMilli(s: []const u8) -> i32 {
    if (s.len == 0) {
        return -1;
    }
    let i: usize = 0;
    let whole: u32 = 0;
    let has_digits: bool = false;
    while (i < s.len && s[i] >= '0' as u8 && s[i] <= '9' as u8) {
        has_digits = true;
        whole = whole * 10 + ((s[i] - ('0' as u8)) as u32);
        if (whole > 1) {
            return -1;
        }
        i += 1;
    }

    let frac: u32 = 0;
    let scale: u32 = 100;
    if (i < s.len && s[i] == '.' as u8) {
        i += 1;
        while (i < s.len && s[i] >= '0' as u8 && s[i] <= '9' as u8) {
            has_digits = true;
            const d: u32 = (s[i] - ('0' as u8)) as u32;
            if (scale > 0) {
                frac = frac + d * scale;
                scale = scale / 10;
            }
            i += 1;
        }
    }
    if (!has_digits || i != s.len) {
        return -1;
    }
    const milli: u32 = whole * 1000 + frac;
    if (milli == 0 || milli > 1000) {
        return -1;
    }
    return milli as i32;
}

pub fn parseMetric(packet: []const u8) -> StatsdMetric {
    const fail = StatsdMetric {
        name_start: 0,
        name_len: 0,
        value: 0,
        kind: STATSD_INVALID,
        sample_rate_milli: 1000,
        tags_start: 0,
        tags_len: 0,
        valid: false,
    };

    if (packet.len == 0) {
        return fail;
    }

    // Locate ':'
    let colon: usize = 0;
    while (colon < packet.len && packet[colon] != ':' as u8) {
        if (packet[colon] == '|' as u8 || packet[colon] == '\n' as u8 || packet[colon] == '\r' as u8) {
            return fail;
        }
        colon += 1;
    }
    if (colon == 0 || colon + 1 >= packet.len) {
        return fail;
    }

    // Locate first '|'
    let pipe1: usize = colon + 1;
    while (pipe1 < packet.len && packet[pipe1] != '|' as u8) {
        pipe1 += 1;
    }
    if (pipe1 >= packet.len) {
        return fail;
    }

    const val_res: IntParseResult = parseI64Slice(packet[(colon + 1)..pipe1]);
    if (!val_res.ok) {
        return fail;
    }

    // Read type segment after pipe1
    let type_end: usize = pipe1 + 1;
    while (type_end < packet.len && packet[type_end] != '|' as u8) {
        type_end += 1;
    }
    const type_slice: []const u8 = packet[(pipe1 + 1)..type_end];
    let kind: u8 = STATSD_INVALID;
    if (bytesEq(type_slice, "c")) {
        kind = STATSD_COUNTER;
    } else if (bytesEq(type_slice, "ms")) {
        kind = STATSD_TIMING;
    } else if (bytesEq(type_slice, "g")) {
        kind = STATSD_GAUGE;
    } else if (bytesEq(type_slice, "s")) {
        kind = STATSD_SET;
    } else {
        return fail;
    }

    let rate_milli: u16 = 1000;
    let tags_start: usize = 0;
    let tags_len: usize = 0;

    let pos: usize = type_end;
    while (pos < packet.len) {
        if (packet[pos] != '|' as u8) {
            return fail;
        }
        pos += 1;
        let seg_end: usize = pos;
        while (seg_end < packet.len && packet[seg_end] != '|' as u8) {
            seg_end += 1;
        }
        if (seg_end <= pos) {
            return fail;
        }
        const prefix: u8 = packet[pos];
        if (prefix == '@' as u8) {
            const parsed_rate: i32 = parseSampleRateMilli(packet[(pos + 1)..seg_end]);
            if (parsed_rate < 0) {
                return fail;
            }
            rate_milli = parsed_rate as u16;
        } else if (prefix == '#' as u8) {
            tags_start = pos + 1;
            tags_len = seg_end - (pos + 1);
        } else {
            return fail;
        }
        pos = seg_end;
    }

    return StatsdMetric {
        name_start: 0,
        name_len: colon,
        value: val_res.value,
        kind: kind,
        sample_rate_milli: rate_milli,
        tags_start: tags_start,
        tags_len: tags_len,
        valid: true,
    };
}
