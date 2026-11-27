// @cap/jcs — Zero-allocation RFC 8785 JSON Canonicalization Scheme (JCS) engine for CP
// Clean-room MIT implementation inspired by npm `fast-json-stable-stringify`, `canonicalize`, and Rust `serde_jcs`.
//
// Features:
// - Deterministic object key sorting by UTF-16 code unit order (RFC 8785 Section 3.2.3).
// - Compact whitespace removal, array order preservation, and strict JSON syntax validation.
// - RFC 8785 string normalization (minimal escapes, `\uXXXX` / surrogate-pair UTF-8 decoding)
//   and numeric normalization (`-0` / `-0.0` -> `0`, trailing fractional zero trimming).
// - `canonicalizeJson(input, out) -> JcsResult` & `areJsonEquivalent(a, b) -> bool`.
// - Accelerated with 16-lane `@Vector(16, u8)` whitespace & string scanning, comptime 256-entry LUTs,
//   and bulk `@memcpy`.

import { isDigit, hexCharLower, startsWith } from "cap:ascii";
import { bytesEq } from "cap:bin";

type JcsVec16 = @Vector(16, u8);

const MAX_DEPTH: u8 = 16;
const MAX_OBJECT_KEYS: usize = 64;

fn buildJsonWsLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[' ' as usize] = 1;
    lut['\t' as usize] = 1;
    lut['\n' as usize] = 1;
    lut['\r' as usize] = 1;
    return lut;
}

fn buildJsonHexLut() -> [256]i8 {
    @setEvalBranchQuota(20000);
    let lut: [256]i8 = [-1; 256];
    let c: usize = 0;
    while (c < 10) {
        lut[('0' as usize) + c] = c as i8;
        c += 1;
    }
    c = 0;
    while (c < 6) {
        lut[('a' as usize) + c] = (10 + c) as i8;
        lut[('A' as usize) + c] = (10 + c) as i8;
        c += 1;
    }
    return lut;
}

const JSON_WS_LUT: [256]u8 = buildJsonWsLut();
const JSON_HEX_LUT: [256]i8 = buildJsonHexLut();

pub struct JcsResult {
    pub ok: bool,
    pub len: usize,
    pub output: string,

    pub static fn fail(out: []u8) -> JcsResult {
        return JcsResult{ ok: false, len: 0, output: out[0..0] };
    }
}

struct ObjEntry {
    key_start: usize,
    key_end: usize,
    val_start: usize,
    val_end: usize,
}

struct StepRes {
    ok: bool,
    next_in: usize,
    next_out: usize,
}

struct SkipRes {
    ok: bool,
    next_in: usize,
}

struct KeyStep {
    ok: bool,
    done: bool,
    unit: u16,
    next_pos: usize,
    pending_low: u16,
}

fn isJsonSpace(c: u8) -> bool {
    return JSON_WS_LUT[c as usize] != 0;
}

fn skipWs(s: string, start: usize) -> usize {
    let i = start;
    const v_sp: JcsVec16 = @splat(' ' as u8);
    const v_tab: JcsVec16 = @splat('\t' as u8);
    const v_nl: JcsVec16 = @splat('\n' as u8);
    const v_cr: JcsVec16 = @splat('\r' as u8);
    while (i + 16 <= s.len) {
        const v: JcsVec16 = s[i .. i + 16][0..16].*;
        const ws = (v == v_sp) | (v == v_tab) | (v == v_nl) | (v == v_cr);
        if (!@reduce(.And, ws)) {
            break;
        }
        i += 16;
    }
    while (i < s.len && JSON_WS_LUT[s[i] as usize] != 0) {
        i += 1;
    }
    return i;
}

fn parseHex4(s: string, pos: usize) -> i32 {
    if (pos + 4 > s.len) {
        return -1;
    }
    const h0 = JSON_HEX_LUT[s[pos] as usize];
    const h1 = JSON_HEX_LUT[s[pos + 1] as usize];
    const h2 = JSON_HEX_LUT[s[pos + 2] as usize];
    const h3 = JSON_HEX_LUT[s[pos + 3] as usize];
    if ((h0 | h1 | h2 | h3) < 0) {
        return -1;
    }
    return ((h0 as i32) << 12) | ((h1 as i32) << 8) | ((h2 as i32) << 4) | (h3 as i32);
}

fn pushByteChecked(out: []u8, pos: usize, b: u8) -> usize? {
    if (pos >= out.len) {
        return null;
    }
    out[pos] = b;
    return pos + 1;
}

fn pushSliceChecked(out: []u8, pos: usize, s: string) -> usize? {
    if (pos + s.len > out.len) {
        return null;
    }
    if (s.len > 0) {
        @memcpy(out[pos .. pos + s.len], s[0..s.len]);
    }
    return pos + s.len;
}

fn writeUtf8Checked(out: []u8, pos: usize, cp: u32) -> usize? {
    if (cp <= 0x7F) {
        return pushByteChecked(out, pos, cp as u8);
    }
    if (cp <= 0x7FF) {
        if (pos + 2 > out.len) {
            return null;
        }
        out[pos] = (0xC0 | (cp >> 6)) as u8;
        out[pos + 1] = (0x80 | (cp & 0x3F)) as u8;
        return pos + 2;
    }
    if (cp <= 0xFFFF) {
        if (pos + 3 > out.len) {
            return null;
        }
        out[pos] = (0xE0 | (cp >> 12)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | (cp & 0x3F)) as u8;
        return pos + 3;
    }
    if (pos + 4 > out.len) {
        return null;
    }
    out[pos] = (0xF0 | (cp >> 18)) as u8;
    out[pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
    out[pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
    out[pos + 3] = (0x80 | (cp & 0x3F)) as u8;
    return pos + 4;
}

fn emitEscapedCodepoint(out: []u8, pos: usize, cp: u32) -> usize? {
    if (cp == 0x22) {
        return pushSliceChecked(out, pos, "\\\"");
    }
    if (cp == 0x5C) {
        return pushSliceChecked(out, pos, "\\\\");
    }
    if (cp == 0x08) {
        return pushSliceChecked(out, pos, "\\b");
    }
    if (cp == 0x0C) {
        return pushSliceChecked(out, pos, "\\f");
    }
    if (cp == 0x0A) {
        return pushSliceChecked(out, pos, "\\n");
    }
    if (cp == 0x0D) {
        return pushSliceChecked(out, pos, "\\r");
    }
    if (cp == 0x09) {
        return pushSliceChecked(out, pos, "\\t");
    }
    if (cp < 0x20) {
        if (pos + 6 > out.len) {
            return null;
        }
        out[pos] = '\\';
        out[pos + 1] = 'u';
        out[pos + 2] = '0';
        out[pos + 3] = '0';
        out[pos + 4] = hexCharLower(((cp >> 4) & 0xF) as u8);
        out[pos + 5] = hexCharLower((cp & 0xF) as u8);
        return pos + 6;
    }
    return writeUtf8Checked(out, pos, cp);
}

/// Validates and skips a JSON string starting at `pos` (`input[pos] == '"'`) using 16-lane SIMD.
fn skipString(input: string, pos: usize) -> SkipRes {
    if (pos >= input.len || input[pos] != '"') {
        return SkipRes{ ok: false, next_in: pos };
    }
    const v_q: JcsVec16 = @splat('"' as u8);
    const v_bs: JcsVec16 = @splat('\\' as u8);
    const v_ctrl: JcsVec16 = @splat(0x20 as u8);
    let i = pos + 1;
    while (i < input.len) {
        while (i + 16 <= input.len) {
            const v: JcsVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == v_q) | (v == v_bs) | (v < v_ctrl))) {
                break;
            }
            i += 16;
        }
        if (i >= input.len) {
            break;
        }
        const c = input[i];
        if (c == '"') {
            return SkipRes{ ok: true, next_in: i + 1 };
        }
        if (c < 0x20) {
            return SkipRes{ ok: false, next_in: i };
        }
        if (c == '\\') {
            if (i + 1 >= input.len) {
                return SkipRes{ ok: false, next_in: i };
            }
            const esc = input[i + 1];
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                i += 2;
                continue;
            }
            if (esc == 'u') {
                if (parseHex4(input, i + 2) < 0) {
                    return SkipRes{ ok: false, next_in: i };
                }
                i += 6;
                continue;
            }
            return SkipRes{ ok: false, next_in: i };
        }
        i += 1;
    }
    return SkipRes{ ok: false, next_in: i };
}

/// Validates and skips a JSON number starting at `pos`.
fn skipNumber(input: string, pos: usize) -> SkipRes {
    let i = pos;
    if (i < input.len && input[i] == '-') {
        i += 1;
    }
    if (i >= input.len) {
        return SkipRes{ ok: false, next_in: i };
    }
    if (input[i] == '0') {
        i += 1;
        // Reject leading zeros like `01`
        if (i < input.len && isDigit(input[i])) {
            return SkipRes{ ok: false, next_in: i };
        }
    } else if (input[i] >= '1' && input[i] <= '9') {
        i += 1;
        while (i < input.len && isDigit(input[i])) {
            i += 1;
        }
    } else {
        return SkipRes{ ok: false, next_in: i };
    }

    if (i < input.len && input[i] == '.') {
        i += 1;
        if (i >= input.len || !isDigit(input[i])) {
            return SkipRes{ ok: false, next_in: i };
        }
        while (i < input.len && isDigit(input[i])) {
            i += 1;
        }
    }

    if (i < input.len && (input[i] == 'e' || input[i] == 'E')) {
        i += 1;
        if (i < input.len && (input[i] == '+' || input[i] == '-')) {
            i += 1;
        }
        if (i >= input.len || !isDigit(input[i])) {
            return SkipRes{ ok: false, next_in: i };
        }
        while (i < input.len && isDigit(input[i])) {
            i += 1;
        }
    }

    return SkipRes{ ok: true, next_in: i };
}

/// Validates and skips any JSON value starting at `pos`.
fn skipValue(input: string, start: usize, depth: u8) -> SkipRes {
    if (depth > MAX_DEPTH) {
        return SkipRes{ ok: false, next_in: start };
    }
    const pos = skipWs(input, start);
    if (pos >= input.len) {
        return SkipRes{ ok: false, next_in: pos };
    }
    const c = input[pos];

    if (c == '"') {
        return skipString(input, pos);
    }
    if (c == '-' || isDigit(c)) {
        return skipNumber(input, pos);
    }
    if (c == 't' && startsWith(input[pos..input.len], "true")) {
        return SkipRes{ ok: true, next_in: pos + 4 };
    }
    if (c == 'f' && startsWith(input[pos..input.len], "false")) {
        return SkipRes{ ok: true, next_in: pos + 5 };
    }
    if (c == 'n' && startsWith(input[pos..input.len], "null")) {
        return SkipRes{ ok: true, next_in: pos + 4 };
    }
    if (c == '[') {
        let i = skipWs(input, pos + 1);
        if (i < input.len && input[i] == ']') {
            return SkipRes{ ok: true, next_in: i + 1 };
        }
        while (true) {
            const elem = skipValue(input, i, depth + 1);
            if (!elem.ok) {
                return elem;
            }
            i = skipWs(input, elem.next_in);
            if (i < input.len && input[i] == ',') {
                i = skipWs(input, i + 1);
                continue;
            }
            if (i < input.len && input[i] == ']') {
                return SkipRes{ ok: true, next_in: i + 1 };
            }
            return SkipRes{ ok: false, next_in: i };
        }
    }
    if (c == '{') {
        let i = skipWs(input, pos + 1);
        if (i < input.len && input[i] == '}') {
            return SkipRes{ ok: true, next_in: i + 1 };
        }
        while (true) {
            const k = skipString(input, i);
            if (!k.ok) {
                return k;
            }
            i = skipWs(input, k.next_in);
            if (i >= input.len || input[i] != ':') {
                return SkipRes{ ok: false, next_in: i };
            }
            const v = skipValue(input, i + 1, depth + 1);
            if (!v.ok) {
                return v;
            }
            i = skipWs(input, v.next_in);
            if (i < input.len && input[i] == ',') {
                i = skipWs(input, i + 1);
                continue;
            }
            if (i < input.len && input[i] == '}') {
                return SkipRes{ ok: true, next_in: i + 1 };
            }
            return SkipRes{ ok: false, next_in: i };
        }
    }

    return SkipRes{ ok: false, next_in: pos };
}

/// Decodes the next UTF-16 code unit from a JSON string interior `[pos..end)` (zero allocation).
fn nextKeyUtf16Unit(input: string, pos: usize, end: usize, pending_low: u16) -> KeyStep {
    if (pending_low != 0) {
        return KeyStep{ ok: true, done: false, unit: pending_low, next_pos: pos, pending_low: 0 };
    }
    if (pos >= end) {
        return KeyStep{ ok: true, done: true, unit: 0, next_pos: pos, pending_low: 0 };
    }
    const c = input[pos];
    if (c == '\\') {
        if (pos + 1 >= end) {
            return KeyStep{ ok: false, done: true, unit: 0, next_pos: pos, pending_low: 0 };
        }
        const esc = input[pos + 1];
        if (esc == '"') { return KeyStep{ ok: true, done: false, unit: 0x22, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == '\\') { return KeyStep{ ok: true, done: false, unit: 0x5C, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == '/') { return KeyStep{ ok: true, done: false, unit: 0x2F, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 'b') { return KeyStep{ ok: true, done: false, unit: 0x08, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 'f') { return KeyStep{ ok: true, done: false, unit: 0x0C, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 'n') { return KeyStep{ ok: true, done: false, unit: 0x0A, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 'r') { return KeyStep{ ok: true, done: false, unit: 0x0D, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 't') { return KeyStep{ ok: true, done: false, unit: 0x09, next_pos: pos + 2, pending_low: 0 }; }
        if (esc == 'u') {
            const u = parseHex4(input, pos + 2);
            if (u < 0) {
                return KeyStep{ ok: false, done: true, unit: 0, next_pos: pos, pending_low: 0 };
            }
            return KeyStep{ ok: true, done: false, unit: u as u16, next_pos: pos + 6, pending_low: 0 };
        }
        return KeyStep{ ok: false, done: true, unit: 0, next_pos: pos, pending_low: 0 };
    }

    if (c < 0x80) {
        return KeyStep{ ok: true, done: false, unit: c as u16, next_pos: pos + 1, pending_low: 0 };
    }
    if ((c & 0xE0) == 0xC0 && pos + 1 < end) {
        const cp = (((c & 0x1F) as u32) << 6) | ((input[pos + 1] & 0x3F) as u32);
        return KeyStep{ ok: true, done: false, unit: cp as u16, next_pos: pos + 2, pending_low: 0 };
    }
    if ((c & 0xF0) == 0xE0 && pos + 2 < end) {
        const cp = (((c & 0x0F) as u32) << 12) | (((input[pos + 1] & 0x3F) as u32) << 6) | ((input[pos + 2] & 0x3F) as u32);
        return KeyStep{ ok: true, done: false, unit: cp as u16, next_pos: pos + 3, pending_low: 0 };
    }
    if ((c & 0xF8) == 0xF0 && pos + 3 < end) {
        const cp = (((c & 0x07) as u32) << 18) | (((input[pos + 1] & 0x3F) as u32) << 12) | (((input[pos + 2] & 0x3F) as u32) << 6) | ((input[pos + 3] & 0x3F) as u32);
        const adj = cp - 0x10000;
        const hi = (0xD800 + (adj >> 10)) as u16;
        const lo = (0xDC00 + (adj & 0x3FF)) as u16;
        return KeyStep{ ok: true, done: false, unit: hi, next_pos: pos + 4, pending_low: lo };
    }
    return KeyStep{ ok: true, done: false, unit: c as u16, next_pos: pos + 1, pending_low: 0 };
}

/// Compares two JSON string keys by RFC 8785 UTF-16 code unit order (zero heap allocation).
fn compareJsonKeys(input: string, s1: usize, e1: usize, s2: usize, e2: usize) -> i8 {
    let p1 = s1 + 1; // skip opening '"'
    let p2 = s2 + 1;
    const end1 = e1 - 1; // exclude closing '"'
    const end2 = e2 - 1;
    let low1: u16 = 0;
    let low2: u16 = 0;

    while (true) {
        const a = nextKeyUtf16Unit(input, p1, end1, low1);
        const b = nextKeyUtf16Unit(input, p2, end2, low2);
        p1 = a.next_pos;
        low1 = a.pending_low;
        p2 = b.next_pos;
        low2 = b.pending_low;

        if (a.done && b.done) {
            return 0;
        }
        if (a.done) {
            return -1;
        }
        if (b.done) {
            return 1;
        }
        if (a.unit < b.unit) {
            return -1;
        }
        if (a.unit > b.unit) {
            return 1;
        }
    }
}

fn emitCanonicalString(input: string, pos: usize, out: []u8, out_pos: usize) -> StepRes {
    if (pos >= input.len || input[pos] != '"') {
        return StepRes{ ok: false, next_in: pos, next_out: out_pos };
    }
    const p0 = pushByteChecked(out, out_pos, '"');
    if (p0 == null) {
        return StepRes{ ok: false, next_in: pos, next_out: out_pos };
    }
    let op = p0.?;
    let i = pos + 1;

    const v_q: JcsVec16 = @splat('"' as u8);
    const v_bs: JcsVec16 = @splat('\\' as u8);
    const v_ctrl: JcsVec16 = @splat(0x20 as u8);

    while (i < input.len) {
        // 16-lane SIMD fast path: scan safe non-escaped characters and bulk-copy via @memcpy
        const run_start = i;
        while (i + 16 <= input.len) {
            const v: JcsVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == v_q) | (v == v_bs) | (v < v_ctrl))) {
                break;
            }
            i += 16;
        }
        if (i > run_start) {
            const run_len = i - run_start;
            if (op + run_len > out.len) {
                return StepRes{ ok: false, next_in: run_start, next_out: op };
            }
            @memcpy(out[op .. op + run_len], input[run_start..i]);
            op += run_len;
            if (i >= input.len) {
                break;
            }
        }

        const c = input[i];
        if (c == '"') {
            const pe = pushByteChecked(out, op, '"');
            if (pe == null) {
                return StepRes{ ok: false, next_in: i, next_out: op };
            }
            return StepRes{ ok: true, next_in: i + 1, next_out: pe.? };
        }
        if (c < 0x20) {
            return StepRes{ ok: false, next_in: i, next_out: op };
        }
        if (c == '\\') {
            if (i + 1 >= input.len) {
                return StepRes{ ok: false, next_in: i, next_out: op };
            }
            const esc = input[i + 1];
            if (esc == '"') {
                const np = pushSliceChecked(out, op, "\\\"");
                if (np == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = np.?;
                i += 2;
                continue;
            }
            if (esc == '\\') {
                const np = pushSliceChecked(out, op, "\\\\");
                if (np == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = np.?;
                i += 2;
                continue;
            }
            if (esc == '/') {
                // RFC 8785: forward slash does not need escaping
                const np = pushByteChecked(out, op, '/');
                if (np == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = np.?;
                i += 2;
                continue;
            }
            if (esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                if (op + 2 > out.len) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                out[op] = '\\';
                out[op + 1] = esc;
                op += 2;
                i += 2;
                continue;
            }
            if (esc == 'u') {
                const unit1 = parseHex4(input, i + 2);
                if (unit1 < 0) {
                    return StepRes{ ok: false, next_in: i, next_out: op };
                }
                let cp: u32 = unit1 as u32;
                let step: usize = 6;
                // Decode UTF-16 surrogate pair if present
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 12 <= input.len && input[i + 6] == '\\' && input[i + 7] == 'u') {
                    const unit2 = parseHex4(input, i + 8);
                    if (unit2 >= 0xDC00 && unit2 <= 0xDFFF) {
                        cp = (0x10000 as u32) + (((cp - (0xD800 as u32)) << 10) | ((unit2 as u32) - (0xDC00 as u32)));
                        step = 12;
                    }
                }
                const np = emitEscapedCodepoint(out, op, cp);
                if (np == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = np.?;
                i += step;
                continue;
            }
            return StepRes{ ok: false, next_in: i, next_out: op };
        }
        const np = pushByteChecked(out, op, c);
        if (np == null) {
            return StepRes{ ok: false, next_in: i, next_out: op };
        }
        op = np.?;
        i += 1;
    }

    return StepRes{ ok: false, next_in: i, next_out: op };
}

fn emitCanonicalNumber(input: string, pos: usize, out: []u8, out_pos: usize) -> StepRes {
    const sk = skipNumber(input, pos);
    if (!sk.ok) {
        return StepRes{ ok: false, next_in: pos, next_out: out_pos };
    }
    const raw = input[pos..sk.next_in];

    // Check if value is mathematically zero (handles -0, -0.0, 0.00, -0e5, etc.)
    let has_nonzero_digit = false;
    let has_exp = false;
    let dot_idx: usize = raw.len;
    let k: usize = 0;
    while (k < raw.len) {
        const c = raw[k];
        if (c == 'e' || c == 'E') {
            has_exp = true;
            break;
        }
        if (c == '.') {
            dot_idx = k;
        } else if (c >= '1' && c <= '9') {
            has_nonzero_digit = true;
        }
        k += 1;
    }

    if (!has_nonzero_digit) {
        const np = pushByteChecked(out, out_pos, '0');
        if (np == null) {
            return StepRes{ ok: false, next_in: pos, next_out: out_pos };
        }
        return StepRes{ ok: true, next_in: sk.next_in, next_out: np.? };
    }

    // For non-exponent decimal numbers, trim redundant trailing zeros after '.'
    if (!has_exp && dot_idx < raw.len) {
        let end = raw.len;
        while (end > dot_idx + 1 && raw[end - 1] == '0') {
            end -= 1;
        }
        if (end == dot_idx + 1) {
            end = dot_idx;
        }
        const np = pushSliceChecked(out, out_pos, raw[0..end]);
        if (np == null) {
            return StepRes{ ok: false, next_in: pos, next_out: out_pos };
        }
        return StepRes{ ok: true, next_in: sk.next_in, next_out: np.? };
    }

    const np = pushSliceChecked(out, out_pos, raw);
    if (np == null) {
        return StepRes{ ok: false, next_in: pos, next_out: out_pos };
    }
    return StepRes{ ok: true, next_in: sk.next_in, next_out: np.? };
}

fn emitCanonicalValue(input: string, start: usize, out: []u8, out_pos: usize, depth: u8) -> StepRes {
    if (depth > MAX_DEPTH) {
        return StepRes{ ok: false, next_in: start, next_out: out_pos };
    }
    const pos = skipWs(input, start);
    if (pos >= input.len) {
        return StepRes{ ok: false, next_in: pos, next_out: out_pos };
    }
    const c = input[pos];

    if (c == '"') {
        return emitCanonicalString(input, pos, out, out_pos);
    }
    if (c == '-' || isDigit(c)) {
        return emitCanonicalNumber(input, pos, out, out_pos);
    }
    if (c == 't' && startsWith(input[pos..input.len], "true")) {
        const np = pushSliceChecked(out, out_pos, "true");
        if (np == null) { return StepRes{ ok: false, next_in: pos, next_out: out_pos }; }
        return StepRes{ ok: true, next_in: pos + 4, next_out: np.? };
    }
    if (c == 'f' && startsWith(input[pos..input.len], "false")) {
        const np = pushSliceChecked(out, out_pos, "false");
        if (np == null) { return StepRes{ ok: false, next_in: pos, next_out: out_pos }; }
        return StepRes{ ok: true, next_in: pos + 5, next_out: np.? };
    }
    if (c == 'n' && startsWith(input[pos..input.len], "null")) {
        const np = pushSliceChecked(out, out_pos, "null");
        if (np == null) { return StepRes{ ok: false, next_in: pos, next_out: out_pos }; }
        return StepRes{ ok: true, next_in: pos + 4, next_out: np.? };
    }

    // Array: preserve element order
    if (c == '[') {
        const p0 = pushByteChecked(out, out_pos, '[');
        if (p0 == null) { return StepRes{ ok: false, next_in: pos, next_out: out_pos }; }
        let op = p0.?;
        let i = skipWs(input, pos + 1);
        if (i < input.len && input[i] == ']') {
            const pe = pushByteChecked(out, op, ']');
            if (pe == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
            return StepRes{ ok: true, next_in: i + 1, next_out: pe.? };
        }
        let first = true;
        while (true) {
            if (!first) {
                const pc = pushByteChecked(out, op, ',');
                if (pc == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = pc.?;
            }
            first = false;
            const ev = emitCanonicalValue(input, i, out, op, depth + 1);
            if (!ev.ok) {
                return ev;
            }
            op = ev.next_out;
            i = skipWs(input, ev.next_in);
            if (i < input.len && input[i] == ',') {
                i = skipWs(input, i + 1);
                continue;
            }
            if (i < input.len && input[i] == ']') {
                const pe = pushByteChecked(out, op, ']');
                if (pe == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                return StepRes{ ok: true, next_in: i + 1, next_out: pe.? };
            }
            return StepRes{ ok: false, next_in: i, next_out: op };
        }
    }

    // Object: collect entries on stack, sort keys by RFC 8785 UTF-16 order, then emit recursively
    if (c == '{') {
        const p0 = pushByteChecked(out, out_pos, '{');
        if (p0 == null) { return StepRes{ ok: false, next_in: pos, next_out: out_pos }; }
        let op = p0.?;
        let i = skipWs(input, pos + 1);
        if (i < input.len && input[i] == '}') {
            const pe = pushByteChecked(out, op, '}');
            if (pe == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
            return StepRes{ ok: true, next_in: i + 1, next_out: pe.? };
        }

        let entries: [MAX_OBJECT_KEYS]ObjEntry = undefined;
        let count: usize = 0;

        while (true) {
            const k_start = i;
            const k_res = skipString(input, k_start);
            if (!k_res.ok) {
                return StepRes{ ok: false, next_in: k_start, next_out: op };
            }
            i = skipWs(input, k_res.next_in);
            if (i >= input.len || input[i] != ':') {
                return StepRes{ ok: false, next_in: i, next_out: op };
            }
            const v_start = skipWs(input, i + 1);
            const v_res = skipValue(input, v_start, depth + 1);
            if (!v_res.ok) {
                return StepRes{ ok: false, next_in: v_start, next_out: op };
            }
            if (count >= MAX_OBJECT_KEYS) {
                return StepRes{ ok: false, next_in: k_start, next_out: op };
            }
            entries[count] = ObjEntry{
                key_start: k_start,
                key_end: k_res.next_in,
                val_start: v_start,
                val_end: v_res.next_in,
            };
            count += 1;

            i = skipWs(input, v_res.next_in);
            if (i < input.len && input[i] == ',') {
                i = skipWs(input, i + 1);
                continue;
            }
            if (i < input.len && input[i] == '}') {
                i += 1;
                break;
            }
            return StepRes{ ok: false, next_in: i, next_out: op };
        }

        // In-place insertion sort of object entries by RFC 8785 UTF-16 key order
        let si: usize = 1;
        while (si < count) {
            const key_entry = entries[si];
            let sj = si;
            while (sj > 0 && compareJsonKeys(
                input,
                key_entry.key_start,
                key_entry.key_end,
                entries[sj - 1].key_start,
                entries[sj - 1].key_end,
            ) < 0) {
                entries[sj] = entries[sj - 1];
                sj -= 1;
            }
            entries[sj] = key_entry;
            si += 1;
        }

        // Emit sorted entries
        let ei: usize = 0;
        while (ei < count) {
            if (ei > 0) {
                const pc = pushByteChecked(out, op, ',');
                if (pc == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
                op = pc.?;
            }
            const ek = emitCanonicalString(input, entries[ei].key_start, out, op);
            if (!ek.ok) {
                return ek;
            }
            const pcol = pushByteChecked(out, ek.next_out, ':');
            if (pcol == null) { return StepRes{ ok: false, next_in: i, next_out: ek.next_out }; }
            const ev = emitCanonicalValue(input, entries[ei].val_start, out, pcol.?, depth + 1);
            if (!ev.ok) {
                return ev;
            }
            op = ev.next_out;
            ei += 1;
        }

        const pe = pushByteChecked(out, op, '}');
        if (pe == null) { return StepRes{ ok: false, next_in: i, next_out: op }; }
        return StepRes{ ok: true, next_in: i, next_out: pe.? };
    }

    return StepRes{ ok: false, next_in: pos, next_out: out_pos };
}

/// Canonicalizes a JSON string according to RFC 8785 (JCS) into caller-provided `out` (zero heap allocation).
pub fn canonicalizeJson(input: string, out: []u8) -> JcsResult {
    const res = emitCanonicalValue(input, 0, out, 0, 0);
    if (!res.ok) {
        return JcsResult.fail(out);
    }
    const trailing = skipWs(input, res.next_in);
    if (trailing != input.len) {
        return JcsResult.fail(out);
    }
    return JcsResult{
        ok: true,
        len: res.next_out,
        output: out[0..res.next_out],
    };
}

/// Checks whether two JSON strings are semantically equivalent (ignoring whitespace and object key order).
/// Supports payloads up to 4096 canonical bytes with zero heap allocation.
pub fn areJsonEquivalent(a: string, b: string) -> bool {
    let buf_a: [4096]u8 = undefined;
    let buf_b: [4096]u8 = undefined;
    const ra = canonicalizeJson(a, buf_a[0..4096]);
    if (!ra.ok) {
        return false;
    }
    const rb = canonicalizeJson(b, buf_b[0..4096]);
    if (!rb.ok) {
        return false;
    }
    return bytesEq(buf_a[0..ra.len], buf_b[0..rb.len]);
}
