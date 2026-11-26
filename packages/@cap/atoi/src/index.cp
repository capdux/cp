// @cap/atoi — Zero-heap overflow-checked integer parsing directly from byte slices
// Matching crates.io Top 550 atoi (262M+ downloads). Clean-room MIT in pure CP.

pub struct ParseU64Result {
    pub value: u64,
    pub used: usize,
    pub ok: bool,
}

pub struct ParseI64Result {
    pub value: i64,
    pub used: usize,
    pub ok: bool,
}

const U64_MAX_VAL: u64 = ((0xffffffff as u64) << 32) | (0xffffffff as u64);
const I64_MAX_U64: u64 = ((0x7fffffff as u64) << 32) | (0xffffffff as u64);
const I64_MIN_MAG: u64 = (1 as u64) << 63;

fn digitVal(ch: u8, radix: u32): i32 {
    let v: i32 = -1;
    if (ch >= 48 && ch <= 57) {
        v = (ch - 48) as i32;
    } else if (ch >= 65 && ch <= 90) {
        v = (ch - 65 + 10) as i32;
    } else if (ch >= 97 && ch <= 122) {
        v = (ch - 97 + 10) as i32;
    }
    if (v >= 0 && (v as u32) < radix) { return v; }
    return -1;
}

// Parses leading unsigned integer in `radix` (2..=36), returning `(value, used_bytes, ok)`.
// Stops at first non-digit byte; returns `ok = false` on empty/no-digit or u64 overflow.
pub fn fromRadixU64(bytes: string, radix: u32): ParseU64Result {
    if (radix < 2 || radix > 36 || bytes.len == 0) {
        return ParseU64Result { value: 0, used: 0, ok: false };
    }
    let rad = radix as u64;
    let limit_div = U64_MAX_VAL / rad;
    let limit_rem = U64_MAX_VAL % rad;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        let d = digitVal(bytes[i], radix);
        if (d < 0) { break; }
        let du = d as u64;
        if (acc > limit_div || (acc == limit_div && du > limit_rem)) {
            return ParseU64Result { value: 0, used: i, ok: false };
        }
        acc = acc * rad + du;
        i = i + 1;
    }
    if (i == 0) {
        return ParseU64Result { value: 0, used: 0, ok: false };
    }
    return ParseU64Result { value: acc, used: i, ok: true };
}

pub fn fromRadix10U64(bytes: string): ParseU64Result {
    return fromRadixU64(bytes, 10);
}

pub fn fromRadix16U64(bytes: string): ParseU64Result {
    return fromRadixU64(bytes, 16);
}

// Parses leading signed integer in `radix` (2..=36) with optional leading `+` or `-`.
// Handles exact i64::MIN (-9223372036854775808) without overflow!
pub fn fromRadixI64(bytes: string, radix: u32): ParseI64Result {
    if (radix < 2 || radix > 36 || bytes.len == 0) {
        return ParseI64Result { value: 0, used: 0, ok: false };
    }
    let i: usize = 0;
    let is_neg = false;
    if (bytes[0] == 45) {
        is_neg = true;
        i = 1;
    } else if (bytes[0] == 43) {
        i = 1;
    }
    if (i >= bytes.len) {
        return ParseI64Result { value: 0, used: 0, ok: false };
    }

    let rad = radix as u64;
    let max_mag = if (is_neg) { I64_MIN_MAG } else { I64_MAX_U64 };
    let limit_div = max_mag / rad;
    let limit_rem = max_mag % rad;

    let acc: u64 = 0;
    let digits: usize = 0;
    while (i < bytes.len) {
        let d = digitVal(bytes[i], radix);
        if (d < 0) { break; }
        let du = d as u64;
        if (acc > limit_div || (acc == limit_div && du > limit_rem)) {
            return ParseI64Result { value: 0, used: i, ok: false };
        }
        acc = acc * rad + du;
        digits = digits + 1;
        i = i + 1;
    }
    if (digits == 0) {
        return ParseI64Result { value: 0, used: 0, ok: false };
    }
    if (is_neg) {
        if (acc == I64_MIN_MAG) {
            let min_i64: i64 = -9223372036854775807 - 1;
            return ParseI64Result { value: min_i64, used: i, ok: true };
        }
        return ParseI64Result { value: -(acc as i64), used: i, ok: true };
    }
    return ParseI64Result { value: acc as i64, used: i, ok: true };
}

pub fn fromRadix10I64(bytes: string): ParseI64Result {
    return fromRadixI64(bytes, 10);
}

// Strict full-slice base-10 parser (fails if any trailing bytes exist).
pub fn atoiU64(s: string): ParseU64Result {
    let r = fromRadixU64(s, 10);
    if (!r.ok || r.used != s.len) {
        return ParseU64Result { value: 0, used: r.used, ok: false };
    }
    return r;
}

pub fn atoiI64(s: string): ParseI64Result {
    let r = fromRadixI64(s, 10);
    if (!r.ok || r.used != s.len) {
        return ParseI64Result { value: 0, used: r.used, ok: false };
    }
    return r;
}
