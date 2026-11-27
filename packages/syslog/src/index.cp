// @cap/syslog — Zero-heap RFC 5424 Syslog parser & formatter
// MIT License (clean-room CP implementation)

pub const SEV_EMERG: u8 = 0;
pub const SEV_ALERT: u8 = 1;
pub const SEV_CRIT: u8 = 2;
pub const SEV_ERR: u8 = 3;
pub const SEV_WARNING: u8 = 4;
pub const SEV_NOTICE: u8 = 5;
pub const SEV_INFO: u8 = 6;
pub const SEV_DEBUG: u8 = 7;

pub const FAC_KERN: u8 = 0;
pub const FAC_USER: u8 = 1;
pub const FAC_DAEMON: u8 = 3;
pub const FAC_AUTH: u8 = 4;
pub const FAC_LOCAL0: u8 = 16;

pub struct SyslogMessage {
    pub facility: u8,
    pub severity: u8,
    pub version: u8,
    pub ts_start: usize,
    pub ts_len: usize,
    pub host_start: usize,
    pub host_len: usize,
    pub app_start: usize,
    pub app_len: usize,
    pub proc_start: usize,
    pub proc_len: usize,
    pub msgid_start: usize,
    pub msgid_len: usize,
    pub msg_start: usize,
    pub msg_len: usize,
    pub valid: bool,
}

pub fn encodePri(facility: u8, severity: u8) -> u8 {
    return (facility * 8) + (severity & 7);
}

pub fn decodeFacility(pri: u8) -> u8 {
    return pri >> 3;
}

pub fn decodeSeverity(pri: u8) -> u8 {
    return pri & 7;
}

struct TokenSpan {
    start: usize,
    len: usize,
    next_pos: usize,
    ok: bool,
}

fn readSpToken(raw: []const u8, pos: usize) -> TokenSpan {
    const fail = TokenSpan { start: 0, len: 0, next_pos: pos, ok: false };
    if (pos >= raw.len || raw[pos] == ' ' as u8) {
        return fail;
    }
    let end: usize = pos;
    while (end < raw.len && raw[end] != ' ' as u8) {
        end += 1;
    }
    if (end >= raw.len) {
        return fail;
    }
    // NILVALUE "-" maps to empty span (len = 0)
    if (end - pos == 1 && raw[pos] == '-' as u8) {
        return TokenSpan { start: 0, len: 0, next_pos: end + 1, ok: true };
    }
    return TokenSpan { start: pos, len: end - pos, next_pos: end + 1, ok: true };
}

pub fn parseRfc5424(raw: []const u8) -> SyslogMessage {
    let fail = SyslogMessage {
        facility: 0,
        severity: 0,
        version: 0,
        ts_start: 0,
        ts_len: 0,
        host_start: 0,
        host_len: 0,
        app_start: 0,
        app_len: 0,
        proc_start: 0,
        proc_len: 0,
        msgid_start: 0,
        msgid_len: 0,
        msg_start: 0,
        msg_len: 0,
        valid: false,
    };

    if (raw.len < 6 || raw[0] != '<' as u8) {
        return fail;
    }

    // Parse <PRI> (1..3 digits, 0..191, no leading zeros except "0")
    let i: usize = 1;
    let pri: u32 = 0;
    let digits: usize = 0;
    while (i < raw.len && raw[i] >= '0' as u8 && raw[i] <= '9' as u8) {
        if (digits == 1 && pri == 0) {
            return fail;
        }
        pri = pri * 10 + ((raw[i] - ('0' as u8)) as u32);
        digits += 1;
        i += 1;
        if (digits > 3) {
            return fail;
        }
    }
    if (digits == 0 || pri > 191 || i >= raw.len || raw[i] != '>' as u8) {
        return fail;
    }
    i += 1;

    // Parse VERSION (RFC 5424: 1..9 followed by SP)
    if (i >= raw.len || raw[i] < '1' as u8 || raw[i] > '9' as u8) {
        return fail;
    }
    const ver: u8 = raw[i] - ('0' as u8);
    i += 1;
    if (i >= raw.len || raw[i] != ' ' as u8) {
        return fail;
    }
    i += 1;

    const ts: TokenSpan = readSpToken(raw, i);
    if (!ts.ok) {
        return fail;
    }
    const host: TokenSpan = readSpToken(raw, ts.next_pos);
    if (!host.ok) {
        return fail;
    }
    const app: TokenSpan = readSpToken(raw, host.next_pos);
    if (!app.ok) {
        return fail;
    }
    const proc: TokenSpan = readSpToken(raw, app.next_pos);
    if (!proc.ok) {
        return fail;
    }
    const msgid: TokenSpan = readSpToken(raw, proc.next_pos);
    if (!msgid.ok) {
        return fail;
    }

    // Parse STRUCTURED-DATA: either "-" or one or more "[...]" elements
    let pos: usize = msgid.next_pos;
    if (pos >= raw.len) {
        return fail;
    }
    if (raw[pos] == '-' as u8) {
        pos += 1;
    } else if (raw[pos] == '[' as u8) {
        while (pos < raw.len && raw[pos] == '[' as u8) {
            pos += 1;
            let in_quote: bool = false;
            let closed: bool = false;
            while (pos < raw.len) {
                const c: u8 = raw[pos];
                if (in_quote) {
                    if (c == '\\' as u8 && pos + 1 < raw.len) {
                        pos += 2;
                        continue;
                    }
                    if (c == '"' as u8) {
                        in_quote = false;
                    }
                } else {
                    if (c == '"' as u8) {
                        in_quote = true;
                    } else if (c == ']' as u8) {
                        pos += 1;
                        closed = true;
                        break;
                    }
                }
                pos += 1;
            }
            if (!closed) {
                return fail;
            }
        }
    } else {
        return fail;
    }

    let msg_start: usize = raw.len;
    let msg_len: usize = 0;
    if (pos < raw.len) {
        if (raw[pos] != ' ' as u8) {
            return fail;
        }
        msg_start = pos + 1;
        msg_len = raw.len - msg_start;
    }

    const pri_u8: u8 = pri as u8;
    return SyslogMessage {
        facility: decodeFacility(pri_u8),
        severity: decodeSeverity(pri_u8),
        version: ver,
        ts_start: ts.start,
        ts_len: ts.len,
        host_start: host.start,
        host_len: host.len,
        app_start: app.start,
        app_len: app.len,
        proc_start: proc.start,
        proc_len: proc.len,
        msgid_start: msgid.start,
        msgid_len: msgid.len,
        msg_start: msg_start,
        msg_len: msg_len,
        valid: true,
    };
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

fn appendFieldOrNil(dst: []u8, pos: usize, field: []const u8) -> i64 {
    if (field.len == 0) {
        return appendSlice(dst, pos, "-");
    }
    return appendSlice(dst, pos, field);
}

fn appendU8Dec(dst: []u8, pos: usize, val: u8) -> i64 {
    let digits: [4]u8 = undefined;
    let count: usize = 0;
    if (val == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u8 = val;
        while (cur > 0) {
            digits[count] = ('0' as u8) + (cur % 10);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > dst.len - pos) {
        return -1;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        dst[p] = digits[k];
        p += 1;
    }
    return p as i64;
}

pub fn formatRfc5424(
    facility: u8,
    severity: u8,
    timestamp: []const u8,
    host: []const u8,
    app: []const u8,
    proc_id: []const u8,
    msg_id: []const u8,
    msg: []const u8,
    dst: []u8,
) -> i64 {
    if (facility > 23 || severity > 7) {
        return -1;
    }
    const pri: u8 = encodePri(facility, severity);

    let p: i64 = appendSlice(dst, 0, "<");
    if (p < 0) {
        return -1;
    }
    p = appendU8Dec(dst, p as usize, pri);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, ">1 ");
    if (p < 0) {
        return -1;
    }
    p = appendFieldOrNil(dst, p as usize, timestamp);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, " ");
    if (p < 0) {
        return -1;
    }
    p = appendFieldOrNil(dst, p as usize, host);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, " ");
    if (p < 0) {
        return -1;
    }
    p = appendFieldOrNil(dst, p as usize, app);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, " ");
    if (p < 0) {
        return -1;
    }
    p = appendFieldOrNil(dst, p as usize, proc_id);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, " ");
    if (p < 0) {
        return -1;
    }
    p = appendFieldOrNil(dst, p as usize, msg_id);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, " -");
    if (p < 0) {
        return -1;
    }
    if (msg.len > 0) {
        p = appendSlice(dst, p as usize, " ");
        if (p < 0) {
            return -1;
        }
        p = appendSlice(dst, p as usize, msg);
        if (p < 0) {
            return -1;
        }
    }
    return p;
}
