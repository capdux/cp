// @cap/base36 — Zero-heap Base36 & arbitrary radix 2..36 u64 codec
// MIT License (clean-room CP implementation)

pub struct ParseU64Result {
    pub value: u64,
    pub valid: bool,
}

fn digitChar(v: u8, uppercase: bool) -> u8 {
    if (v < 10) {
        return ('0' as u8) + v;
    }
    if (uppercase) {
        return ('A' as u8) + (v - 10);
    }
    return ('a' as u8) + (v - 10);
}

fn digitVal(c: u8) -> i32 {
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return (c - ('a' as u8) + 10) as i32;
    }
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return (c - ('A' as u8) + 10) as i32;
    }
    return -1;
}

pub fn encodeRadix(val: u64, radix: u8, uppercase: bool, out: []u8) -> usize {
    if (radix < 2 || radix > 36) {
        return 0;
    }
    let digits: [64]u8 = undefined;
    let count: usize = 0;
    const base: u64 = radix as u64;

    if (val == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u64 = val;
        while (cur > 0) {
            const rem: u8 = (cur % base) as u8;
            digits[count] = digitChar(rem, uppercase);
            count += 1;
            cur = cur / base;
        }
    }

    if (count > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[pos] = digits[k];
        pos += 1;
    }
    return pos;
}

pub fn decodeRadix(input: string, radix: u8) -> ParseU64Result {
    const fail = ParseU64Result { value: 0, valid: false };
    if (radix < 2 || radix > 36 || input.len == 0) {
        return fail;
    }

    const max_u64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const base: u64 = radix as u64;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < input.len) {
        const d_i: i32 = digitVal(input[i]);
        if (d_i < 0 || d_i >= (radix as i32)) {
            return fail;
        }
        const d: u64 = d_i as u64;
        if (acc > (max_u64 - d) / base) {
            return fail;
        }
        acc = acc * base + d;
        i += 1;
    }
    return ParseU64Result { value: acc, valid: true };
}

pub fn encodeBase36(val: u64, out: []u8) -> usize {
    return encodeRadix(val, 36, false, out);
}

pub fn decodeBase36(input: string) -> ParseU64Result {
    return decodeRadix(input, 36);
}

pub fn isValidBase36(input: string) -> bool {
    return decodeRadix(input, 36).valid;
}
