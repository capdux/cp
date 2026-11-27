// Source: cp/lib/fmt/index.cp (Built-in Standard Library: cap:fmt)
// Zero-heap buffered formatting and strict integer parsing primitives.

const LFMT_HEX_LOWER: string = "0123456789abcdef";
const LFMT_HEX_UPPER: string = "0123456789ABCDEF";

fn lfmtWriteByte(b: u8, out: []u8, pos: usize): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn lfmtWriteStr(s: string, out: []u8, pos: usize): usize {
    if (pos >= out.len) return pos;
    let rem = out.len - pos;
    let n = if (s.len < rem) s.len else rem;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = s[i];
    }
    return pos + n;
}

fn lfmtWriteRepeat(ch: u8, count: usize, out: []u8, pos: usize): usize {
    if (pos >= out.len) return pos;
    let rem = out.len - pos;
    let n = if (count < rem) count else rem;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = ch;
    }
    return pos + n;
}

fn lfmtWriteU64(val: u64, out: []u8, pos: usize): usize {
    if (val == 0) {
        return lfmtWriteByte(48, out, pos);
    }
    let digits: [24]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        let d = @as(u8, @intCast(v % 10));
        digits[n] = 48 + d;
        n += 1;
        v = v / 10;
    }
    let p = pos;
    let i = n;
    while (i > 0) {
        i -= 1;
        p = lfmtWriteByte(digits[i], out, p);
    }
    return p;
}

fn lfmtWriteI64(val: i64, out: []u8, pos: usize): usize {
    if (val < 0) {
        let p = lfmtWriteByte(45, out, pos);
        let mag = @as(u64, @intCast(-(val + 1))) + 1;
        return lfmtWriteU64(mag, out, p);
    }
    return lfmtWriteU64(@as(u64, @intCast(val)), out, pos);
}

fn lfmtWriteHexU64(val: u64, uppercase: bool, out: []u8, pos: usize): usize {
    if (val == 0) {
        return lfmtWriteByte(48, out, pos);
    }
    let table = if (uppercase) LFMT_HEX_UPPER else LFMT_HEX_LOWER;
    let digits: [24]u8 = undefined;
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        let idx = @as(usize, @intCast(v & 15));
        digits[n] = table[idx];
        n += 1;
        v = v >> 4;
    }
    let p = pos;
    let i = n;
    while (i > 0) {
        i -= 1;
        p = lfmtWriteByte(digits[i], out, p);
    }
    return p;
}

fn lfmtWriteHexByte(b: u8, uppercase: bool, out: []u8, pos: usize): usize {
    let table = if (uppercase) LFMT_HEX_UPPER else LFMT_HEX_LOWER;
    let hi = @as(usize, (b >> 4) & 15);
    let lo = @as(usize, b & 15);
    let p = lfmtWriteByte(table[hi], out, pos);
    return lfmtWriteByte(table[lo], out, p);
}

fn lfmtPow10(exp: u8): u64 {
    let res: u64 = 1;
    let i: u8 = 0;
    while (i < exp) : (i += 1) {
        res = res * 10;
    }
    return res;
}

fn lfmtWriteFloatFixed(val: f64, decimals: u8, out: []u8, pos: usize): usize {
    if (std.math.isNan(val)) {
        return lfmtWriteStr("NaN", out, pos);
    }
    if (std.math.isInf(val)) {
        if (val < 0.0) {
            return lfmtWriteStr("-Infinity", out, pos);
        }
        return lfmtWriteStr("Infinity", out, pos);
    }
    let p = pos;
    let v = val;
    if (v < 0.0) {
        p = lfmtWriteByte(45, out, p);
        v = -v;
    }
    let places: u8 = if (decimals > 18) 18 else decimals;
    // Guard against values beyond u64 integer range
    if (v >= 1.844674407370955e19) {
        return lfmtWriteStr("Infinity", out, p);
    }
    let int_f = @floor(v);
    let frac_f = v - int_f;
    let int_u = @as(u64, @intFromFloat(int_f));
    let scale = lfmtPow10(places);
    let scaled_f = frac_f * @as(f64, @floatFromInt(scale)) + 0.5;
    let frac_u = @as(u64, @intFromFloat(scaled_f));
    if (frac_u >= scale) {
        frac_u -= scale;
        int_u += 1;
    }
    p = lfmtWriteU64(int_u, out, p);
    if (places == 0) {
        return p;
    }
    p = lfmtWriteByte(46, out, p);
    let digits: [24]u8 = undefined;
    let count = @as(usize, places);
    let f = frac_u;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        let d = @as(u8, @intCast(f % 10));
        digits[i] = 48 + d;
        f = f / 10;
    }
    let j = count;
    while (j > 0) {
        j -= 1;
        p = lfmtWriteByte(digits[j], out, p);
    }
    return p;
}

fn lfmtWriteJsonStr(s: string, out: []u8, pos: usize): usize {
    let p = lfmtWriteByte(34, out, pos); // '"'
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        let b = s[i];
        if (b == 34) {
            p = lfmtWriteStr("\\\"", out, p);
        } else if (b == 92) {
            p = lfmtWriteStr("\\\\", out, p);
        } else if (b == 8) {
            p = lfmtWriteStr("\\b", out, p);
        } else if (b == 12) {
            p = lfmtWriteStr("\\f", out, p);
        } else if (b == 10) {
            p = lfmtWriteStr("\\n", out, p);
        } else if (b == 13) {
            p = lfmtWriteStr("\\r", out, p);
        } else if (b == 9) {
            p = lfmtWriteStr("\\t", out, p);
        } else if (b < 32) {
            p = lfmtWriteStr("\\u00", out, p);
            p = lfmtWriteHexByte(b, false, out, p);
        } else {
            p = lfmtWriteByte(b, out, p);
        }
    }
    return lfmtWriteByte(34, out, p);
}

fn lfmtHexVal(b: u8): u8? {
    if (b >= 48 && b <= 57) return b - 48;
    if (b >= 65 && b <= 70) return b - 65 + 10;
    if (b >= 97 && b <= 102) return b - 97 + 10;
    return null;
}

/// Zero-heap fixed-buffer writer for formatting text, numbers, hex, floats, and JSON strings.
pub struct BufWriter {
    out: []u8,
    pos: usize,

    static fn new(out: []u8): BufWriter {
        return BufWriter{ out: out, pos: 0 };
    }

    fn written(): usize {
        return _.pos;
    }

    fn remaining(): usize {
        if (_.pos < _.out.len) {
            return _.out.len - _.pos;
        }
        return 0;
    }

    fn asString(): string {
        return _.out[0.._.pos];
    }

    fn writeByte(*_, b: u8) {
        _.pos = lfmtWriteByte(b, _.out, _.pos);
    }

    fn writeStr(*_, s: string) {
        _.pos = lfmtWriteStr(s, _.out, _.pos);
    }

    fn writeRepeat(*_, ch: u8, count: usize) {
        _.pos = lfmtWriteRepeat(ch, count, _.out, _.pos);
    }

    fn writeU64(*_, val: u64) {
        _.pos = lfmtWriteU64(val, _.out, _.pos);
    }

    fn writeI64(*_, val: i64) {
        _.pos = lfmtWriteI64(val, _.out, _.pos);
    }

    fn writeHexU64(*_, val: u64, uppercase: bool) {
        _.pos = lfmtWriteHexU64(val, uppercase, _.out, _.pos);
    }

    fn writeHexByte(*_, b: u8, uppercase: bool) {
        _.pos = lfmtWriteHexByte(b, uppercase, _.out, _.pos);
    }

    fn writeFloatFixed(*_, val: f64, decimals: u8) {
        _.pos = lfmtWriteFloatFixed(val, decimals, _.out, _.pos);
    }

    fn writeJsonStr(*_, s: string) {
        _.pos = lfmtWriteJsonStr(s, _.out, _.pos);
    }
}

/// Free-function convenience wrappers returning the updated cursor position.
pub fn writeStr(s: string, out: []u8, pos: usize): usize {
    return lfmtWriteStr(s, out, pos);
}

pub fn writeU64(val: u64, out: []u8, pos: usize): usize {
    return lfmtWriteU64(val, out, pos);
}

pub fn writeI64(val: i64, out: []u8, pos: usize): usize {
    return lfmtWriteI64(val, out, pos);
}

pub fn writeHexByte(b: u8, uppercase: bool, out: []u8, pos: usize): usize {
    return lfmtWriteHexByte(b, uppercase, out, pos);
}

/// Zero-heap strict decimal u64 parser (accepts optional leading '+', rejects invalid chars & overflow).
pub fn parseU64(s: string): u64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    if (s[0] == 43) { // '+'
        i = 1;
        if (i >= s.len) return null;
    }
    let acc: u64 = 0;
    while (i < s.len) : (i += 1) {
        let b = s[i];
        if (b < 48 || b > 57) return null;
        let d = @as(u64, b - 48);
        if (acc > 1844674407370955161) return null;
        if (acc == 1844674407370955161 && d > 5) return null;
        acc = acc * 10 + d;
    }
    return acc;
}

/// Zero-heap strict decimal i64 parser (accepts optional '+'/'-', overflow-safe including i64 min).
pub fn parseI64(s: string): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == 45) { // '-'
        neg = true;
        i = 1;
    } else if (s[0] == 43) { // '+'
        i = 1;
    }
    if (i >= s.len) return null;
    let acc: u64 = 0;
    let max_last: u64 = if (neg) 8 else 7;
    while (i < s.len) : (i += 1) {
        let b = s[i];
        if (b < 48 || b > 57) return null;
        let d = @as(u64, b - 48);
        if (acc > 922337203685477580) return null;
        if (acc == 922337203685477580 && d > max_last) return null;
        acc = acc * 10 + d;
    }
    if (neg) {
        if (acc == 0) return 0;
        let pos_m1 = @as(i64, @intCast(acc - 1));
        return -pos_m1 - 1;
    }
    return @as(i64, @intCast(acc));
}

/// Zero-heap strict decimal u32 parser.
pub fn parseU32(s: string): u32? {
    if (v := parseU64(s)) {
        if (v > 4294967295) return null;
        return @as(u32, @intCast(v));
    }
    return null;
}

/// Zero-heap strict decimal u16 parser.
pub fn parseU16(s: string): u16? {
    if (v := parseU64(s)) {
        if (v > 65535) return null;
        return @as(u16, @intCast(v));
    }
    return null;
}

/// Zero-heap strict hexadecimal u64 parser (accepts optional leading '+' and optional '0x'/'0X').
pub fn parseHexU64(s: string): u64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    if (s[0] == 43) { // '+'
        i = 1;
        if (i >= s.len) return null;
    }
    if (i + 2 <= s.len && s[i] == 48 && (s[i + 1] == 120 || s[i + 1] == 88)) {
        i += 2;
        if (i >= s.len) return null;
    }
    let acc: u64 = 0;
    while (i < s.len) : (i += 1) {
        if (d := lfmtHexVal(s[i])) {
            if ((acc >> 60) != 0) return null;
            acc = (acc << 4) | @as(u64, d);
        } else {
            return null;
        }
    }
    return acc;
}
