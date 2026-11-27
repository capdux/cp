// @cap/morgan-log — Zero-heap HTTP access log template formatting engine.
// Clean-room MIT CP implementation modeled on npm `morgan` v1.10.

import { strEq, isAlnum, findByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub const MORGAN_TINY: string = ":method :url :status :res[content-length] - :response-time ms";
pub const MORGAN_DEV: string = ":method :url :status :response-time ms - :res[content-length]";
pub const MORGAN_SHORT: string = ":remote-addr :remote-user :method :url HTTP/:http-version :status :res[content-length] - :response-time ms";
pub const MORGAN_COMMON: string = ":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length]";
pub const MORGAN_COMBINED: string = ":remote-addr - :remote-user [:date] \":method :url HTTP/:http-version\" :status :res[content-length] \":referrer\" \":user-agent\"";

fn mrgIsTokenChar(c: u8): bool {
    return isAlnum(c) || c == 45 || c == 95;
}

fn mrgWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn mrgWriteOrDash(out: []u8, pos: usize, s: string): usize {
    if (s.len == 0) {
        return mrgWriteByte(out, pos, 45); // '-'
    }
    return writeStr(s, out, pos);
}

fn mrgWriteResponseTime(out: []u8, pos: usize, us: u64): usize {
    let ms_int = us / 1000;
    let ms_frac = us % 1000;
    let w = writeU64(ms_int, out, pos);
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
            const b_start = i + 1;
            if (b_end := findByte(format_str, 93, b_start)) {
                arg = format_str[b_start..b_end];
                has_arg = true;
                i = b_end + 1;
            }
        }

        if (strEq(name, "remote-addr")) {
            w = mrgWriteOrDash(out, w, ev.remote_addr);
        } else if (strEq(name, "remote-user")) {
            w = mrgWriteOrDash(out, w, ev.remote_user);
        } else if (strEq(name, "date")) {
            w = mrgWriteOrDash(out, w, ev.date);
        } else if (strEq(name, "method")) {
            w = mrgWriteOrDash(out, w, ev.method);
        } else if (strEq(name, "url")) {
            w = mrgWriteOrDash(out, w, ev.url);
        } else if (strEq(name, "http-version")) {
            w = mrgWriteOrDash(out, w, ev.http_version);
        } else if (strEq(name, "status")) {
            if (ev.status == 0) {
                w = mrgWriteByte(out, w, 45);
            } else {
                w = writeU64(ev.status as u64, out, w);
            }
        } else if (strEq(name, "res") && has_arg && strEq(arg, "content-length")) {
            if (ev.has_content_length) {
                w = writeU64(ev.content_length, out, w);
            } else {
                w = mrgWriteByte(out, w, 45);
            }
        } else if (strEq(name, "referrer") || strEq(name, "referer")) {
            w = mrgWriteOrDash(out, w, ev.referrer);
        } else if (strEq(name, "user-agent")) {
            w = mrgWriteOrDash(out, w, ev.user_agent);
        } else if (strEq(name, "response-time")) {
            w = mrgWriteResponseTime(out, w, ev.response_time_us);
        } else {
            // Preserve unknown ':...' or literal colon sequence verbatim
            w = writeStr(format_str[colon_pos..i], out, w);
        }
    }

    return w;
}

/// Formats `ev` using a named preset (`"tiny"`, `"dev"`, `"short"`, `"common"`, `"combined"`)
/// or falls back to treating `preset` as a custom format string.
pub fn formatPreset(preset: string, ev: HttpLogEvent, out: []u8): usize {
    if (strEq(preset, "tiny")) {
        return formatMorganLog(MORGAN_TINY, ev, out);
    }
    if (strEq(preset, "dev")) {
        return formatMorganLog(MORGAN_DEV, ev, out);
    }
    if (strEq(preset, "short")) {
        return formatMorganLog(MORGAN_SHORT, ev, out);
    }
    if (strEq(preset, "common")) {
        return formatMorganLog(MORGAN_COMMON, ev, out);
    }
    if (strEq(preset, "combined")) {
        return formatMorganLog(MORGAN_COMBINED, ev, out);
    }
    return formatMorganLog(preset, ev, out);
}
