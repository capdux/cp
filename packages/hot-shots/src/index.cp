// @cap/statsd — Zero-heap StatsD & DogStatsD UDP metric codec
// MIT License (clean-room CP implementation)

import { isDigit } from "cap:ascii";
import { bytesEq, findByteInBytes } from "cap:bin";
import { writeStr, writeI64, parseI64 } from "cap:fmt";

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
    return writeStr(s, dst, pos) as i64;
}

fn appendI64(dst: []u8, pos: usize, v: i64) -> i64 {
    let tmp: [24]u8 = undefined;
    const n: usize = writeI64(v, tmp[0..24], 0);
    return appendSlice(dst, pos, tmp[0..n]);
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

fn parseSampleRateMilli(s: []const u8) -> i32 {
    if (s.len == 0) {
        return -1;
    }
    let i: usize = 0;
    let whole: u32 = 0;
    let has_digits: bool = false;
    while (i < s.len && isDigit(s[i])) {
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
        while (i < s.len && isDigit(s[i])) {
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
    let pipe1: usize = 0;
    if (p1 := findByteInBytes(packet, '|' as u8, colon + 1)) {
        pipe1 = p1;
    } else {
        return fail;
    }

    let val: i64 = 0;
    if (v := parseI64(packet[(colon + 1)..pipe1])) {
        val = v;
    } else {
        return fail;
    }

    // Read type segment after pipe1
    const type_end: usize = findByteInBytes(packet, '|' as u8, pipe1 + 1) ?? packet.len;
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
        const seg_end: usize = findByteInBytes(packet, '|' as u8, pos) ?? packet.len;
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
        value: val,
        kind: kind,
        sample_rate_milli: rate_milli,
        tags_start: tags_start,
        tags_len: tags_len,
        valid: true,
    };
}
