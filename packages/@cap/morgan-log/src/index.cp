// @cap/morgan-log — Zero-heap HTTP access log template formatting engine.
// Clean-room MIT CP implementation modeled on npm `morgan` v1.10.
// Reuses `formatU64` from `@cap/itoa` for fast zero-heap numeric formatting.

import { formatU64 } from "../../itoa/src/index.cp";

pub const MORGAN_TINY: string = ":method :url :status :res[content-length] - :response-time ms";
pub const MORGAN_DEV: string = ":method :url :status :response-time ms - :res[content-length]";
pub const MORGAN_SHORT: string = ":remote-addr :remote-user :method :url HTTP/:http-version :status :res[content-length] - :response-time ms";
pub const MORGAN_COMMON: string = ":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length]";
pub const MORGAN_COMBINED: string = ":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length] \":referrer\" \":user-agent\"";

fn mrgStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn mrgIsTokenChar(c: u8): bool {
    return (c >= 97 && c <= 122)
        || (c >= 65 && c <= 90)
        || (c >= 48 && c <= 57)
        || c == 45
        || c == 95;
}

fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn mrgWriteSlice(out: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize {
    if (s.len == 0) {
        return mrgWriteByte(out, pos, 45); // '-'
    }
    return mrgWriteSlice(out, pos, s);
}

fn mrgWriteU64(out: []u8, pos: usize, val: u64): usize {
    let tmp: [24]u8 = undefined;
    let n = formatU64(val, tmp[0..24]);
    let w = pos;
    let i: usize = 0;
    while (i < n && w < out.len) {
        out[w] = tmp[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize {
    let ms_int = us / 1000;
    let ms_frac = us % 1000;
    let w = mrgWriteU64(out, pos, ms_int);
    w = mrgWriteByte(out, w, 46); // '.'
    w = mrgWriteByte(out, w, (48 + ((ms_frac / 100) % 10)) as u8);
    w = mrgWriteByte(out, w, (48 + ((ms_frac / 10) % 10)) as u8);
    w = mrgWriteByte(out, w, (48 + (ms_frac % 10)) as u8);
    return w;
}

/// Zero-heap HTTP access log event payload.
pub struct HttpLogEvent {
    pub remote_addr: string,
    pub remote_user: string,
    pub date: string,
    pub method: string,
    pub url: string,
    pub http_version: string,
    pub status: u16,
    pub content_length: u64,
    pub has_content_length: bool,
    pub referrer: string,
    pub user_agent: string,
    pub response_time_us: u64,
}

/// Formats `ev` according to a Morgan format template `format_str` into `out`.
/// Missing string fields and absent content-length render as `"-"` per Apache CLF/Morgan rules.
/// Returns total bytes written.
pub fn formatMorganLog(format_str: string, ev: HttpLogEvent, out: []u8): usize {
    let w: usize = 0;
    let i: usize = 0;

    while (i < format_str.len && w < out.len) {
        if (format_str[i] != 58) { // ':'
            w = mrgWriteByte(out, w, format_str[i]);
            i = i + 1;
            continue;
        }

        // Parse ':token' and optional '[arg]'
        let colon_pos = i;
        i = i + 1;
        let name_start = i;
        while (i < format_str.len && mrgIsTokenChar(format_str[i])) {
            i = i + 1;
        }
        let name = format_str[name_start..i];

        let arg: string = "";
        let has_arg = false;
        if (i < format_str.len && format_str[i] == 91) { // '['
            let b_start = i + 1;
            let b_end = b_start;
            while (b_end < format_str.len && format_str[b_end] != 93) {
                b_end = b_end + 1;
            }
            if (b_end < format_str.len) {
                arg = format_str[b_start..b_end];
                has_arg = true;
                i = b_end + 1;
            }
        }

        if (mrgStrEq(name, "remote-addr")) {
            w = mrgWriteOrDash(out, w, ev.remote_addr);
        } else if (mrgStrEq(name, "remote-user")) {
            w = mrgWriteOrDash(out, w, ev.remote_user);
        } else if (mrgStrEq(name, "date")) {
            w = mrgWriteOrDash(out, w, ev.date);
        } else if (mrgStrEq(name, "method")) {
            w = mrgWriteOrDash(out, w, ev.method);
        } else if (mrgStrEq(name, "url")) {
            w = mrgWriteOrDash(out, w, ev.url);
        } else if (mrgStrEq(name, "http-version")) {
            w = mrgWriteOrDash(out, w, ev.http_version);
        } else if (mrgStrEq(name, "status")) {
            if (ev.status == 0) {
                w = mrgWriteByte(out, w, 45);
            } else {
                w = mrgWriteU64(out, w, ev.status as u64);
            }
        } else if (mrgStrEq(name, "res") && has_arg && mrgStrEq(arg, "content-length")) {
            if (ev.has_content_length) {
                w = mrgWriteU64(out, w, ev.content_length);
            } else {
                w = mrgWriteByte(out, w, 45);
            }
        } else if (mrgStrEq(name, "referrer") || mrgStrEq(name, "referer")) {
            w = mrgWriteOrDash(out, w, ev.referrer);
        } else if (mrgStrEq(name, "user-agent")) {
            w = mrgWriteOrDash(out, w, ev.user_agent);
        } else if (mrgStrEq(name, "response-time")) {
            w = mrgWriteResponseTime(out, w, ev.response_time_us);
        } else {
            // Preserve unknown ':...' or literal colon sequence verbatim
            w = mrgWriteSlice(out, w, format_str[colon_pos..i]);
        }
    }

    return w;
}

/// Formats `ev` using a named preset (`"tiny"`, `"dev"`, `"short"`, `"common"`, `"combined"`)
/// or falls back to treating `preset` as a custom format string.
pub fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize {
    if (mrgStrEq(preset, "tiny")) {
        return formatMorganLog(MORGAN_TINY, ev, out);
    }
    if (mrgStrEq(preset, "dev")) {
        return formatMorganLog(MORGAN_DEV, ev, out);
    }
    if (mrgStrEq(preset, "short")) {
        return formatMorganLog(MORGAN_SHORT, ev, out);
    }
    if (mrgStrEq(preset, "common")) {
        return formatMorganLog(MORGAN_COMMON, ev, out);
    }
    if (mrgStrEq(preset, "combined")) {
        return formatMorganLog(MORGAN_COMBINED, ev, out);
    }
    return formatMorganLog(preset, ev, out);
}
