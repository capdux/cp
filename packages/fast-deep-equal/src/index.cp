// @cap/fast-deep-equal — Zero-heap semantic JSON deep equality & type classifier in pure CP
// MIT License — Clean-room implementation inspired by npm `fast-deep-equal` / `kind-of`

pub const KIND_INVALID: u8 = 0;
pub const KIND_NULL: u8 = 1;
pub const KIND_BOOL: u8 = 2;
pub const KIND_NUMBER: u8 = 3;
pub const KIND_STRING: u8 = 4;
pub const KIND_ARRAY: u8 = 5;
pub const KIND_OBJECT: u8 = 6;

const MAX_DEPTH: u32 = 64;

struct ValueSpan {
    start: usize,
    end: usize,
    kind: u8,
    valid: bool,
}

struct DecChar {
    code: u32,
    next_pos: usize,
    done: bool,
}

fn isWs(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 13 || ch == 10;
}

fn skipWs(s: string, pos: usize): usize {
    let i: usize = pos;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    return i;
}

fn matchLit(s: string, pos: usize, lit: string): bool {
    if (pos + lit.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (s[pos + i] != lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn hexDigit(ch: u8): i32 {
    if (ch >= 48 && ch <= 57) { return (ch - 48) as i32; }
    if (ch >= 65 && ch <= 70) { return (ch - 55) as i32; }
    if (ch >= 97 && ch <= 102) { return (ch - 87) as i32; }
    return -1;
}

fn invalidSpan(): ValueSpan {
    return ValueSpan { start: 0, end: 0, kind: KIND_INVALID, valid: false };
}

fn scanString(s: string, pos: usize): ValueSpan {
    if (pos >= s.len || s[pos] != 34) {
        return invalidSpan();
    }
    let i: usize = pos + 1;
    while (i < s.len) {
        const ch: u8 = s[i];
        if (ch < 0x20) {
            return invalidSpan();
        }
        if (ch == 34) {
            return ValueSpan { start: pos, end: i + 1, kind: KIND_STRING, valid: true };
        }
        if (ch == 92) { // '\\'
            if (i + 1 >= s.len) {
                return invalidSpan();
            }
            const esc: u8 = s[i + 1];
            if (esc == 117) { // \uXXXX
                if (i + 5 >= s.len) {
                    return invalidSpan();
                }
                if (hexDigit(s[i + 2]) < 0 || hexDigit(s[i + 3]) < 0 ||
                    hexDigit(s[i + 4]) < 0 || hexDigit(s[i + 5]) < 0) {
                    return invalidSpan();
                }
                i += 6;
                continue;
            }
            if (esc == 34 || esc == 92 || esc == 47 || esc == 98 ||
                esc == 102 || esc == 110 || esc == 114 || esc == 116) {
                i += 2;
                continue;
            }
            return invalidSpan();
        }
        i += 1;
    }
    return invalidSpan();
}

fn scanNumber(s: string, pos: usize): ValueSpan {
    let i: usize = pos;
    if (i < s.len && s[i] == 45) { // '-'
        i += 1;
    }
    if (i >= s.len) {
        return invalidSpan();
    }
    if (s[i] == 48) { // '0'
        i += 1;
    } else if (s[i] >= 49 && s[i] <= 57) {
        i += 1;
        while (i < s.len && s[i] >= 48 && s[i] <= 57) {
            i += 1;
        }
    } else {
        return invalidSpan();
    }
    if (i < s.len && s[i] == 46) { // '.'
        i += 1;
        if (i >= s.len || s[i] < 48 || s[i] > 57) {
            return invalidSpan();
        }
        while (i < s.len && s[i] >= 48 && s[i] <= 57) {
            i += 1;
        }
    }
    if (i < s.len && (s[i] == 101 || s[i] == 69)) { // 'e' or 'E'
        i += 1;
        if (i < s.len && (s[i] == 43 || s[i] == 45)) {
            i += 1;
        }
        if (i >= s.len || s[i] < 48 || s[i] > 57) {
            return invalidSpan();
        }
        while (i < s.len && s[i] >= 48 && s[i] <= 57) {
            i += 1;
        }
    }
    return ValueSpan { start: pos, end: i, kind: KIND_NUMBER, valid: true };
}

fn parseValueSpan(s: string, pos: usize, depth: u32): ValueSpan {
    if (depth > MAX_DEPTH) {
        return invalidSpan();
    }
    const p: usize = skipWs(s, pos);
    if (p >= s.len) {
        return invalidSpan();
    }
    const ch: u8 = s[p];
    if (ch == 110) { // null
        if (matchLit(s, p, "null")) {
            return ValueSpan { start: p, end: p + 4, kind: KIND_NULL, valid: true };
        }
        return invalidSpan();
    }
    if (ch == 116) { // true
        if (matchLit(s, p, "true")) {
            return ValueSpan { start: p, end: p + 4, kind: KIND_BOOL, valid: true };
        }
        return invalidSpan();
    }
    if (ch == 102) { // false
        if (matchLit(s, p, "false")) {
            return ValueSpan { start: p, end: p + 5, kind: KIND_BOOL, valid: true };
        }
        return invalidSpan();
    }
    if (ch == 34) {
        return scanString(s, p);
    }
    if (ch == 45 || (ch >= 48 && ch <= 57)) {
        return scanNumber(s, p);
    }
    if (ch == 91) { // '['
        let i: usize = skipWs(s, p + 1);
        if (i < s.len && s[i] == 93) {
            return ValueSpan { start: p, end: i + 1, kind: KIND_ARRAY, valid: true };
        }
        while (true) {
            const elem: ValueSpan = parseValueSpan(s, i, depth + 1);
            if (!elem.valid) {
                return invalidSpan();
            }
            i = skipWs(s, elem.end);
            if (i >= s.len) {
                return invalidSpan();
            }
            if (s[i] == 93) {
                return ValueSpan { start: p, end: i + 1, kind: KIND_ARRAY, valid: true };
            }
            if (s[i] != 44) {
                return invalidSpan();
            }
            i = skipWs(s, i + 1);
        }
    }
    if (ch == 123) { // '{'
        let i: usize = skipWs(s, p + 1);
        if (i < s.len && s[i] == 125) {
            return ValueSpan { start: p, end: i + 1, kind: KIND_OBJECT, valid: true };
        }
        while (true) {
            const key: ValueSpan = scanString(s, i);
            if (!key.valid) {
                return invalidSpan();
            }
            i = skipWs(s, key.end);
            if (i >= s.len || s[i] != 58) { // ':'
                return invalidSpan();
            }
            const val: ValueSpan = parseValueSpan(s, i + 1, depth + 1);
            if (!val.valid) {
                return invalidSpan();
            }
            i = skipWs(s, val.end);
            if (i >= s.len) {
                return invalidSpan();
            }
            if (s[i] == 125) {
                return ValueSpan { start: p, end: i + 1, kind: KIND_OBJECT, valid: true };
            }
            if (s[i] != 44) {
                return invalidSpan();
            }
            i = skipWs(s, i + 1);
        }
    }
    return invalidSpan();
}

/// Return the JSON root value type (`KIND_INVALID`..`KIND_OBJECT`).
pub fn kindOfJson(s: string): u8 {
    const sp: ValueSpan = parseValueSpan(s, 0, 0);
    if (!sp.valid) {
        return KIND_INVALID;
    }
    const tail: usize = skipWs(s, sp.end);
    if (tail != s.len) {
        return KIND_INVALID;
    }
    return sp.kind;
}

/// Return true iff `s` is a valid JSON object (`{ ... }`).
pub fn isPlainObjectJson(s: string): bool {
    return kindOfJson(s) == KIND_OBJECT;
}

fn parseJsonNumberF64(s: string): f64 {
    let i: usize = 0;
    let is_neg: bool = false;
    if (i < s.len && s[i] == 45) {
        is_neg = true;
        i += 1;
    }
    let val: f64 = 0.0;
    while (i < s.len && s[i] >= 48 && s[i] <= 57) {
        val = val * 10.0 + ((s[i] - 48) as f64);
        i += 1;
    }
    if (i < s.len && s[i] == 46) {
        i += 1;
        let frac_scale: f64 = 0.1;
        while (i < s.len && s[i] >= 48 && s[i] <= 57) {
            val = val + ((s[i] - 48) as f64) * frac_scale;
            frac_scale = frac_scale * 0.1;
            i += 1;
        }
    }
    if (i < s.len && (s[i] == 101 || s[i] == 69)) {
        i += 1;
        let exp_neg: bool = false;
        if (i < s.len && s[i] == 45) {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == 43) {
            i += 1;
        }
        let exp_val: u32 = 0;
        while (i < s.len && s[i] >= 48 && s[i] <= 57) {
            if (exp_val < 400) {
                exp_val = exp_val * 10 + ((s[i] - 48) as u32);
            }
            i += 1;
        }
        let k: u32 = 0;
        while (k < exp_val) {
            if (exp_neg) {
                val = val * 0.1;
            } else {
                val = val * 10.0;
            }
            k += 1;
        }
    }
    return if (is_neg) -val else val;
}

fn nextDecodedChar(s: string, pos: usize): DecChar {
    const end_quote: usize = s.len - 1;
    if (pos >= end_quote) {
        return DecChar { code: 0, next_pos: pos, done: true };
    }
    const ch: u8 = s[pos];
    if (ch != 92) {
        return DecChar { code: ch as u32, next_pos: pos + 1, done: false };
    }
    const esc: u8 = s[pos + 1];
    if (esc == 34) { return DecChar { code: 34, next_pos: pos + 2, done: false }; }
    if (esc == 92) { return DecChar { code: 92, next_pos: pos + 2, done: false }; }
    if (esc == 47) { return DecChar { code: 47, next_pos: pos + 2, done: false }; }
    if (esc == 98) { return DecChar { code: 8, next_pos: pos + 2, done: false }; }
    if (esc == 102) { return DecChar { code: 12, next_pos: pos + 2, done: false }; }
    if (esc == 110) { return DecChar { code: 10, next_pos: pos + 2, done: false }; }
    if (esc == 114) { return DecChar { code: 13, next_pos: pos + 2, done: false }; }
    if (esc == 116) { return DecChar { code: 9, next_pos: pos + 2, done: false }; }
    if (esc == 117) {
        const d0: u32 = hexDigit(s[pos + 2]) as u32;
        const d1: u32 = hexDigit(s[pos + 3]) as u32;
        const d2: u32 = hexDigit(s[pos + 4]) as u32;
        const d3: u32 = hexDigit(s[pos + 5]) as u32;
        const cp: u32 = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
        return DecChar { code: cp, next_pos: pos + 6, done: false };
    }
    return DecChar { code: esc as u32, next_pos: pos + 2, done: false };
}

fn jsonStringsEqual(a: string, b: string): bool {
    let pa: usize = 1;
    let pb: usize = 1;
    while (true) {
        const ca: DecChar = nextDecodedChar(a, pa);
        const cb: DecChar = nextDecodedChar(b, pb);
        if (ca.done && cb.done) {
            return true;
        }
        if (ca.done != cb.done || ca.code != cb.code) {
            return false;
        }
        pa = ca.next_pos;
        pb = cb.next_pos;
    }
}

fn countObjectMembers(obj: string): usize {
    let i: usize = skipWs(obj, 1);
    if (i < obj.len && obj[i] == 125) {
        return 0;
    }
    let count: usize = 0;
    while (i < obj.len) {
        const k: ValueSpan = scanString(obj, i);
        i = skipWs(obj, k.end);
        const v: ValueSpan = parseValueSpan(obj, i + 1, 0);
        count += 1;
        i = skipWs(obj, v.end);
        if (obj[i] == 125) {
            break;
        }
        i = skipWs(obj, i + 1);
    }
    return count;
}

fn arraysDeepEqual(a: string, b: string, depth: u32): bool {
    let ia: usize = skipWs(a, 1);
    let ib: usize = skipWs(b, 1);
    if (a[ia] == 93 || b[ib] == 93) {
        return a[ia] == 93 && b[ib] == 93;
    }
    while (true) {
        const ea: ValueSpan = parseValueSpan(a, ia, depth + 1);
        const eb: ValueSpan = parseValueSpan(b, ib, depth + 1);
        if (!ea.valid || !eb.valid || ea.kind != eb.kind) {
            return false;
        }
        if (!valuesDeepEqual(a[ea.start..ea.end], b[eb.start..eb.end], ea.kind, depth + 1)) {
            return false;
        }
        ia = skipWs(a, ea.end);
        ib = skipWs(b, eb.end);
        if (a[ia] == 93 || b[ib] == 93) {
            return a[ia] == 93 && b[ib] == 93;
        }
        ia = skipWs(a, ia + 1);
        ib = skipWs(b, ib + 1);
    }
}

fn objectsDeepEqual(a: string, b: string, depth: u32): bool {
    const na: usize = countObjectMembers(a);
    const nb: usize = countObjectMembers(b);
    if (na != nb) {
        return false;
    }
    if (na == 0) {
        return true;
    }

    let ia: usize = skipWs(a, 1);
    while (ia < a.len) {
        const ka: ValueSpan = scanString(a, ia);
        const colon_a: usize = skipWs(a, ka.end);
        const va: ValueSpan = parseValueSpan(a, colon_a + 1, depth + 1);
        const key_a_str: string = a[ka.start..ka.end];

        // Search for matching key in object `b` (order-independent)
        let found: bool = false;
        let ib: usize = skipWs(b, 1);
        while (ib < b.len) {
            const kb: ValueSpan = scanString(b, ib);
            const colon_b: usize = skipWs(b, kb.end);
            const vb: ValueSpan = parseValueSpan(b, colon_b + 1, depth + 1);
            if (jsonStringsEqual(key_a_str, b[kb.start..kb.end])) {
                if (va.kind != vb.kind) {
                    return false;
                }
                if (!valuesDeepEqual(a[va.start..va.end], b[vb.start..vb.end], va.kind, depth + 1)) {
                    return false;
                }
                found = true;
                break;
            }
            ib = skipWs(b, vb.end);
            if (b[ib] == 125) {
                break;
            }
            ib = skipWs(b, ib + 1);
        }

        if (!found) {
            return false;
        }

        ia = skipWs(a, va.end);
        if (a[ia] == 125) {
            break;
        }
        ia = skipWs(a, ia + 1);
    }
    return true;
}

fn valuesDeepEqual(a: string, b: string, kind: u8, depth: u32): bool {
    if (kind == KIND_NULL) {
        return true;
    }
    if (kind == KIND_BOOL) {
        return a[0] == b[0]; // 't' vs 'f'
    }
    if (kind == KIND_NUMBER) {
        const fa: f64 = parseJsonNumberF64(a);
        const fb: f64 = parseJsonNumberF64(b);
        return fa == fb;
    }
    if (kind == KIND_STRING) {
        return jsonStringsEqual(a, b);
    }
    if (kind == KIND_ARRAY) {
        return arraysDeepEqual(a, b, depth);
    }
    if (kind == KIND_OBJECT) {
        return objectsDeepEqual(a, b, depth);
    }
    return false;
}

/// Semantic deep equality comparison of two JSON documents without heap allocation.
/// Ignores whitespace formatting and object key order, compares numbers numerically
/// (`1.0 == 1`, `1e2 == 100`, `-0 == 0`), and decodes string escape sequences.
/// Returns `false` if either input is invalid JSON.
pub fn jsonDeepEqual(a: string, b: string): bool {
    const sa: ValueSpan = parseValueSpan(a, 0, 0);
    const sb: ValueSpan = parseValueSpan(b, 0, 0);
    if (!sa.valid || !sb.valid) {
        return false;
    }
    if (skipWs(a, sa.end) != a.len || skipWs(b, sb.end) != b.len) {
        return false;
    }
    if (sa.kind != sb.kind) {
        return false;
    }
    return valuesDeepEqual(a[sa.start..sa.end], b[sb.start..sb.end], sa.kind, 0);
}
