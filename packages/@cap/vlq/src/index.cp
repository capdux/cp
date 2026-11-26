// @cap/vlq — Zero-heap Source Map v3 Base64 VLQ signed integer codec
// MIT License (clean-room CP implementation)

pub struct DecodeResult {
    pub value: i64,
    pub consumed: usize,
    pub valid: bool,
}

fn b64Char(v: u8) -> u8 {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    return alphabet[v as usize];
}

fn b64Val(c: u8) -> i32 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return (c - ('A' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return (c - ('a' as u8) + 26) as i32;
    }
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8) + 52) as i32;
    }
    if (c == '+' as u8) {
        return 62;
    }
    if (c == '/' as u8) {
        return 63;
    }
    return -1;
}

pub fn encodeOne(val: i64, out: []u8) -> usize {
    const max_mag: u64 = ((0x3FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    let vlq: u64 = 0;
    if (val < 0) {
        const mag: u64 = ((-(val + 1)) as u64) + 1;
        if (mag > max_mag) {
            return 0;
        }
        vlq = (mag << 1) | 1;
    } else {
        const mag: u64 = val as u64;
        if (mag > max_mag) {
            return 0;
        }
        vlq = mag << 1;
    }

    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return 0;
        }
        let digit: u8 = (vlq & 0x1F) as u8;
        vlq = vlq >> 5;
        if (vlq > 0) {
            digit = digit | 0x20;
        }
        out[pos] = b64Char(digit);
        pos += 1;
        if (vlq == 0) {
            break;
        }
    }
    return pos;
}

pub fn decodeOne(input: string, start: usize) -> DecodeResult {
    const fail = DecodeResult { value: 0, consumed: 0, valid: false };
    if (start >= input.len) {
        return fail;
    }

    let result: u64 = 0;
    let shift: u32 = 0;
    let pos: usize = start;

    while (true) {
        if (pos >= input.len) {
            return fail;
        }
        const d: i32 = b64Val(input[pos]);
        if (d < 0) {
            return fail;
        }
        pos += 1;

        const cont: bool = (d & 0x20) != 0;
        const chunk: u64 = (d & 0x1F) as u64;

        // Guard against 64-bit shift overflow (max 12 chunks = 60 bits)
        if (shift > 60 || (shift == 60 && chunk > 7)) {
            return fail;
        }
        result = result | (chunk << (shift as u6));
        shift += 5;

        if (!cont) {
            break;
        }
    }

    const is_neg: bool = (result & 1) == 1;
    const mag: u64 = result >> 1;
    let val: i64 = mag as i64;
    if (is_neg) {
        val = -(mag as i64);
    }
    return DecodeResult {
        value: val,
        consumed: pos - start,
        valid: true,
    };
}

pub fn encodeSlice(values: []const i64, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < values.len) {
        const w: usize = encodeOne(values[i], out[pos..out.len]);
        if (w == 0) {
            return 0;
        }
        pos += w;
        i += 1;
    }
    return pos;
}

pub fn decodeSlice(input: string, out: []i64) -> i64 {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < input.len) {
        if (count >= out.len) {
            return -1;
        }
        const r: DecodeResult = decodeOne(input, pos);
        if (!r.valid) {
            return -1;
        }
        out[count] = r.value;
        count += 1;
        pos += r.consumed;
    }
    return count as i64;
}
