// @cap/to-regex-range — Zero-allocation numeric range to compact regex compiler
// MIT License

import { writeU64 } from "cap:fmt";

const MAX_I64_POS_U64: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
const MAX_I64_NEG_U64: u64 = (1 as u64) << 63;

struct TrrBufWriter {
    pos: usize,
    overflow: bool,
}

fn bwPush(w: *TrrBufWriter, out: []u8, ch: u8) {
    if (w.*.overflow) {
        return;
    }
    if (w.*.pos >= out.len) {
        w.*.overflow = true;
        return;
    }
    out[w.*.pos] = ch;
    w.*.pos = w.*.pos + 1;
}

pub fn countDigits(n: u64): usize {
    if (n == 0) {
        return 1;
    }
    let v: u64 = n;
    let d: usize = 0;
    while (v > 0) {
        v = v / 10;
        d = d + 1;
    }
    return d;
}

fn absI64ToU64(v: i64): u64 {
    if (v >= 0) {
        return v as u64;
    }
    // Safe for i64 min without overflow
    let shifted: i64 = v + 1;
    let mag: u64 = (0 - shifted) as u64;
    return mag + 1;
}

fn trrWriteU64(w: *TrrBufWriter, out: []u8, v: u64) {
    let digits: [24]u8 = undefined;
    let n: usize = writeU64(v, digits[0..24], 0);
    let i: usize = 0;
    while (i < n) {
        bwPush(w, out, digits[i]);
        i = i + 1;
    }
}

fn isSinglePattern(a: u64, b: u64): bool {
    let da: [24]u8 = undefined;
    let db: [24]u8 = undefined;
    let na: usize = writeU64(a, da[0..24], 0);
    let nb: usize = writeU64(b, db[0..24], 0);
    if (na != nb) {
        return false;
    }
    let i: usize = 0;
    while (i < na && da[i] == db[i]) {
        i = i + 1;
    }
    if (i >= na) {
        return true;
    }
    // After the first differing digit, all remaining digits must be 0..9
    let j: usize = i + 1;
    while (j < na) {
        if (da[j] != 48 || db[j] != 57) {
            return false;
        }
        j = j + 1;
    }
    return true;
}

fn emitSubRange(w: *TrrBufWriter, out: []u8, a: u64, b: u64) {
    let da: [24]u8 = undefined;
    let db: [24]u8 = undefined;
    let na: usize = writeU64(a, da[0..24], 0);
    * = writeU64(b, db[0..24], 0);
    let i: usize = 0;
    while (i < na) {
        if (da[i] == db[i]) {
            bwPush(w, out, da[i]);
        } else {
            bwPush(w, out, 91); // '['
            bwPush(w, out, da[i]);
            bwPush(w, out, 45); // '-'
            bwPush(w, out, db[i]);
            bwPush(w, out, 93); // ']'
        }
        i = i + 1;
    }
}

fn insertStop(stops: []u64, count: *usize, val: u64) {
    let c: usize = count.*;
    let i: usize = 0;
    while (i < c) {
        if (stops[i] == val) {
            return;
        }
        i = i + 1;
    }
    if (c < stops.len) {
        stops[c] = val;
        count.* = c + 1;
    }
}

fn sortStops(stops: []u64, count: usize) {
    let i: usize = 1;
    while (i < count) {
        let key: u64 = stops[i];
        let j: usize = i;
        while (j > 0 && stops[j - 1] > key) {
            stops[j] = stops[j - 1];
            j = j - 1;
        }
        stops[j] = key;
        i = i + 1;
    }
}

fn splitPositiveStops(a: u64, b: u64, stops: []u64): usize {
    let count: usize = 0;
    if (isSinglePattern(a, b)) {
        insertStop(stops, &count, b);
        return count;
    }

    // Phase 1: upward stops from a (replacing trailing digits with 9s)
    let pow10: u64 = 10;
    let k: usize = 0;
    while (k < 19) {
        let rem: u64 = a % pow10;
        let add_part: u64 = (pow10 - 1) - rem;
        // Avoid u64 overflow: check add_part < b - a
        if (add_part < b - a) {
            let stop: u64 = a + add_part;
            if (stop < b) {
                insertStop(stops, &count, stop);
            }
        } else {
            break;
        }
        if (k == 18) {
            break;
        }
        pow10 = pow10 * 10;
        k = k + 1;
    }

    // Phase 2: downward stops from b (replacing trailing digits with 0s, minus 1)
    let pow10_b: u64 = 10;
    let m: usize = 0;
    while (m < 19) {
        let rem_b: u64 = b % pow10_b;
        let base_b: u64 = b - rem_b;
        if (base_b > a) {
            let stop_b: u64 = base_b - 1;
            if (stop_b >= a && stop_b < b) {
                insertStop(stops, &count, stop_b);
            }
        } else {
            break;
        }
        if (m == 18) {
            break;
        }
        pow10_b = pow10_b * 10;
        m = m + 1;
    }

    insertStop(stops, &count, b);
    sortStops(stops, count);
    return count;
}

fn emitPositiveRange(w: *TrrBufWriter, out: []u8, a: u64, b: u64) {
    let stops: [64]u64 = undefined;
    let count: usize = splitPositiveStops(a, b, stops[0..64]);
    let cur_lo: u64 = a;
    let i: usize = 0;
    while (i < count) {
        if (i > 0) {
            bwPush(w, out, 124); // '|'
        }
        emitSubRange(w, out, cur_lo, stops[i]);
        if (i + 1 < count) {
            cur_lo = stops[i] + 1;
        }
        i = i + 1;
    }
}

fn emitNegativeRange(w: *TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64) {
    let stops: [64]u64 = undefined;
    let count: usize = splitPositiveStops(mag_lo, mag_hi, stops[0..64]);
    bwPush(w, out, 45); // '-'
    if (count > 1) {
        bwPush(w, out, 40); // '('
    }
    let cur_lo: u64 = mag_lo;
    let i: usize = 0;
    while (i < count) {
        if (i > 0) {
            bwPush(w, out, 124); // '|'
        }
        emitSubRange(w, out, cur_lo, stops[i]);
        if (i + 1 < count) {
            cur_lo = stops[i] + 1;
        }
        i = i + 1;
    }
    if (count > 1) {
        bwPush(w, out, 41); // ')'
    }
}

pub fn toRegexRange(min_val: i64, max_val: i64, out: []u8): usize {
    let lo: i64 = min_val;
    let hi: i64 = max_val;
    if (lo > hi) {
        lo = max_val;
        hi = min_val;
    }

    let w: TrrBufWriter = TrrBufWriter { pos: 0, overflow: false };

    if (lo == hi) {
        if (lo < 0) {
            bwPush(&w, out, 45); // '-'
        }
        trrWriteU64(&w, out, absI64ToU64(lo));
    } else if (lo >= 0) {
        emitPositiveRange(&w, out, lo as u64, hi as u64);
    } else if (hi < 0) {
        emitNegativeRange(&w, out, absI64ToU64(hi), absI64ToU64(lo));
    } else {
        // Mixed: negative [lo..-1] and non-negative [0..hi]
        emitNegativeRange(&w, out, 1, absI64ToU64(lo));
        bwPush(&w, out, 124); // '|'
        emitPositiveRange(&w, out, 0, hi as u64);
    }

    if (w.overflow) {
        return 0;
    }
    return w.pos;
}

pub fn matchNumericRange(s: string, min_val: i64, max_val: i64): bool {
    if (s.len == 0) {
        return false;
    }
    let lo: i64 = min_val;
    let hi: i64 = max_val;
    if (lo > hi) {
        lo = max_val;
        hi = min_val;
    }

    let neg: bool = false;
    let idx: usize = 0;
    if (s[0] == 45) {
        neg = true;
        idx = 1;
    }
    if (idx >= s.len) {
        return false;
    }
    let digits_len: usize = s.len - idx;
    // Reject "-0" and leading zeros like "01", "-01"
    if (s[idx] == 48) {
        if (neg || digits_len > 1) {
            return false;
        }
    }

    let limit: u64 = MAX_I64_POS_U64;
    if (neg) {
        limit = MAX_I64_NEG_U64;
    }

    let acc: u64 = 0;
    let i: usize = idx;
    while (i < s.len) {
        let ch: u8 = s[i];
        if (ch < 48 || ch > 57) {
            return false;
        }
        let d: u64 = (ch - 48) as u64;
        if (acc > (limit - d) / 10) {
            return false;
        }
        acc = acc * 10 + d;
        i = i + 1;
    }

    let val: i64 = 0;
    if (neg) {
        if (acc == MAX_I64_NEG_U64) {
            val = (0 - (MAX_I64_POS_U64 as i64)) - 1;
        } else {
            val = 0 - (acc as i64);
        }
    } else {
        val = acc as i64;
    }
    return val >= lo && val <= hi;
}
