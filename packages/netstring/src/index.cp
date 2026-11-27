// @cap/netstring — Zero-heap D. J. Bernstein Netstring Framing (`<len>:<payload>,`)
// Clean-room MIT implementation in pure CP (binary-safe, zero heap allocations).

pub const NS_OK: i32 = 0;
pub const NS_INCOMPLETE: i32 = 1;
pub const NS_INVALID: i32 = -1;

pub struct NetstringFrame {
    status: i32,
    payload_start: usize,
    payload_len: usize,
    consumed: usize,
}

// Compute exact encoded byte length for a payload of `payload_len` bytes
pub fn encodedLen(payload_len: usize): usize {
    let digits: usize = 1;
    let v: usize = payload_len;
    while (v >= 10) {
        digits += 1;
        v = v / 10;
    }
    return digits + 1 + payload_len + 1;
}

// Encode `payload` as `<len>:<payload>,` into `dst`. Returns bytes written, or -1 if `dst` is too small.
pub fn encode(payload: []const u8, dst: []u8): i64 {
    let total: usize = encodedLen(payload.len);
    if (dst.len < total) {
        return -1;
    }
    let tmp: [24]u8 = undefined;
    let dlen: usize = 0;
    let v: usize = payload.len;
    if (v == 0) {
        tmp[0] = '0';
        dlen = 1;
    } else {
        while (v > 0) {
            tmp[dlen] = ('0' as u8) + ((v % 10) as u8);
            dlen += 1;
            v = v / 10;
        }
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < dlen) {
        dst[pos] = tmp[dlen - 1 - i];
        pos += 1;
        i += 1;
    }
    dst[pos] = ':';
    pos += 1;
    let j: usize = 0;
    while (j < payload.len) {
        dst[pos] = payload[j];
        pos += 1;
        j += 1;
    }
    dst[pos] = ',';
    pos += 1;
    return pos as i64;
}

// Decode one netstring frame from `buf` starting at offset 0 (zero-copy payload span).
// Strictly rejects leading zeros like "01:a," per Bernstein's specification.
pub fn decode(buf: []const u8): NetstringFrame {
    if (buf.len == 0) {
        return NetstringFrame { status: NS_INCOMPLETE, payload_start: 0, payload_len: 0, consumed: 0 };
    }
    let pos: usize = 0;
    let plen: usize = 0;
    let digits: usize = 0;
    let first_zero: bool = false;

    while (pos < buf.len && buf[pos] != ':') {
        let c: u8 = buf[pos];
        if (c < '0' || c > '9') {
            return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
        }
        if (digits == 0 && c == '0') {
            first_zero = true;
        } else if (first_zero) {
            // Leading zero followed by another digit is illegal
            return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
        }
        if (digits >= 9) {
            return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
        }
        plen = plen * 10 + ((c - '0') as usize);
        digits += 1;
        pos += 1;
    }

    if (pos >= buf.len) {
        if (digits == 0) {
            return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
        }
        return NetstringFrame { status: NS_INCOMPLETE, payload_start: 0, payload_len: 0, consumed: 0 };
    }
    if (digits == 0) {
        return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
    }
    // Skip ':'
    let pstart: usize = pos + 1;
    if (pstart + plen + 1 > buf.len) {
        return NetstringFrame { status: NS_INCOMPLETE, payload_start: 0, payload_len: 0, consumed: 0 };
    }
    if (buf[pstart + plen] != ',') {
        return NetstringFrame { status: NS_INVALID, payload_start: 0, payload_len: 0, consumed: 0 };
    }
    return NetstringFrame {
        status: NS_OK,
        payload_start: pstart,
        payload_len: plen,
        consumed: pstart + plen + 1,
    };
}
