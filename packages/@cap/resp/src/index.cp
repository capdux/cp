// @cap/resp — Zero-heap Redis RESP2 streaming protocol codec
// MIT License (clean-room CP implementation)

pub const RESP_INCOMPLETE: u8 = 0;
pub const RESP_ERROR_SYNTAX: u8 = 1;
pub const RESP_SIMPLE_STRING: u8 = 2;
pub const RESP_ERR: u8 = 3;
pub const RESP_INTEGER: u8 = 4;
pub const RESP_BULK_STRING: u8 = 5;
pub const RESP_NULL: u8 = 6;
pub const RESP_ARRAY: u8 = 7;

pub struct RespFrame {
    pub kind: u8,
    pub data_start: usize,
    pub data_len: usize,
    pub int_val: i64,
    pub array_len: i64,
    pub consumed: usize,
}

fn makeFrame(kind: u8) -> RespFrame {
    return RespFrame {
        kind: kind,
        data_start: 0,
        data_len: 0,
        int_val: 0,
        array_len: 0,
        consumed: 0,
    };
}

struct IntResult {
    value: i64,
    ok: bool,
}

fn parseI64(s: []const u8) -> IntResult {
    const fail = IntResult { value: 0, ok: false };
    if (s.len == 0) {
        return fail;
    }
    let i: usize = 0;
    let is_neg: bool = false;
    if (s[0] == '-' as u8) {
        is_neg = true;
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
            return IntResult { value: -(max_pos as i64) - 1, ok: true };
        }
        return IntResult { value: -(acc as i64), ok: true };
    }
    return IntResult { value: acc as i64, ok: true };
}

fn findCrlf(buf: []const u8, start: usize) -> i64 {
    let i: usize = start;
    while (i + 1 < buf.len) {
        if (buf[i] == '\r' as u8 && buf[i + 1] == '\n' as u8) {
            return i as i64;
        }
        i += 1;
    }
    return -1;
}

pub fn parseFrame(buf: []const u8) -> RespFrame {
    if (buf.len == 0) {
        return makeFrame(RESP_INCOMPLETE);
    }

    const prefix: u8 = buf[0];
    const is_valid_prefix: bool = prefix == '+' as u8
        || prefix == '-' as u8
        || prefix == ':' as u8
        || prefix == '$' as u8
        || prefix == '*' as u8;
    if (!is_valid_prefix) {
        return makeFrame(RESP_ERROR_SYNTAX);
    }

    const crlf_i: i64 = findCrlf(buf, 1);
    if (crlf_i < 0) {
        return makeFrame(RESP_INCOMPLETE);
    }
    const crlf: usize = crlf_i as usize;
    const line_end: usize = crlf + 2;

    if (prefix == '+' as u8) {
        return RespFrame {
            kind: RESP_SIMPLE_STRING,
            data_start: 1,
            data_len: crlf - 1,
            int_val: 0,
            array_len: 0,
            consumed: line_end,
        };
    }

    if (prefix == '-' as u8) {
        return RespFrame {
            kind: RESP_ERR,
            data_start: 1,
            data_len: crlf - 1,
            int_val: 0,
            array_len: 0,
            consumed: line_end,
        };
    }

    if (prefix == ':' as u8) {
        const ir: IntResult = parseI64(buf[1..crlf]);
        if (!ir.ok) {
            return makeFrame(RESP_ERROR_SYNTAX);
        }
        return RespFrame {
            kind: RESP_INTEGER,
            data_start: 0,
            data_len: 0,
            int_val: ir.value,
            array_len: 0,
            consumed: line_end,
        };
    }

    if (prefix == '$' as u8) {
        const lr: IntResult = parseI64(buf[1..crlf]);
        if (!lr.ok || lr.value < -1) {
            return makeFrame(RESP_ERROR_SYNTAX);
        }
        if (lr.value == -1) {
            return RespFrame {
                kind: RESP_NULL,
                data_start: 0,
                data_len: 0,
                int_val: 0,
                array_len: 0,
                consumed: line_end,
            };
        }
        const bulk_len: usize = lr.value as usize;
        if (bulk_len + 2 > buf.len - line_end) {
            return makeFrame(RESP_INCOMPLETE);
        }
        const tail: usize = line_end + bulk_len;
        if (buf[tail] != '\r' as u8 || buf[tail + 1] != '\n' as u8) {
            return makeFrame(RESP_ERROR_SYNTAX);
        }
        return RespFrame {
            kind: RESP_BULK_STRING,
            data_start: line_end,
            data_len: bulk_len,
            int_val: 0,
            array_len: 0,
            consumed: tail + 2,
        };
    }

    // prefix == '*'
    const ar: IntResult = parseI64(buf[1..crlf]);
    if (!ar.ok || ar.value < -1) {
        return makeFrame(RESP_ERROR_SYNTAX);
    }
    if (ar.value == -1) {
        return RespFrame {
            kind: RESP_NULL,
            data_start: 0,
            data_len: 0,
            int_val: 0,
            array_len: -1,
            consumed: line_end,
        };
    }
    return RespFrame {
        kind: RESP_ARRAY,
        data_start: 0,
        data_len: 0,
        int_val: 0,
        array_len: ar.value,
        consumed: line_end,
    };
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

pub fn encodeSimpleString(s: []const u8, dst: []u8) -> i64 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\r' as u8 || s[i] == '\n' as u8) {
            return -1;
        }
        i += 1;
    }
    let p: i64 = appendByte(dst, 0, '+' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, s);
    if (p < 0) {
        return -1;
    }
    return appendSlice(dst, p as usize, "\r\n");
}

pub fn encodeInteger(v: i64, dst: []u8) -> i64 {
    let p: i64 = appendByte(dst, 0, ':' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendI64(dst, p as usize, v);
    if (p < 0) {
        return -1;
    }
    return appendSlice(dst, p as usize, "\r\n");
}

pub fn encodeNull(dst: []u8) -> i64 {
    return appendSlice(dst, 0, "$-1\r\n");
}

pub fn encodeBulkString(s: []const u8, dst: []u8) -> i64 {
    let p: i64 = appendByte(dst, 0, '$' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendI64(dst, p as usize, s.len as i64);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, "\r\n");
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, s);
    if (p < 0) {
        return -1;
    }
    return appendSlice(dst, p as usize, "\r\n");
}

pub fn encodeCommand(args: []const []const u8, dst: []u8) -> i64 {
    let p: i64 = appendByte(dst, 0, '*' as u8);
    if (p < 0) {
        return -1;
    }
    p = appendI64(dst, p as usize, args.len as i64);
    if (p < 0) {
        return -1;
    }
    p = appendSlice(dst, p as usize, "\r\n");
    if (p < 0) {
        return -1;
    }

    let i: usize = 0;
    while (i < args.len) {
        const cur: usize = p as usize;
        const w: i64 = encodeBulkString(args[i], dst[cur..dst.len]);
        if (w < 0) {
            return -1;
        }
        p = (cur + (w as usize)) as i64;
        i += 1;
    }
    return p;
}
