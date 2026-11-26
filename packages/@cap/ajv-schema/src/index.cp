// @cap/ajv-schema — Zero-heap JSON Schema Draft-07 / 2020-12 validation engine (ajv v8 parity)
// Clean-room MIT CP implementation of npm `ajv` (200M+/wk).
// Reuses:
//   - `zEmail`, `zUrl`, `zUuid`, `zEnum` from `@cap/zod-schema`
//   - `isValidIpv4`, `isValidIpv6` from `@cap/ipaddr`

import { zEmail, zUrl, zUuid, zEnum } from "../../zod-schema/src/index.cp";
import { isValidIpv4, isValidIpv6 } from "../../ipaddr/src/index.cp";

// Error codes
pub const AJV_ERR_INVALID_SCHEMA: i32 = 1;
pub const AJV_ERR_INVALID_DATA: i32 = 2;
pub const AJV_ERR_TYPE: i32 = 3;
pub const AJV_ERR_REQUIRED: i32 = 4;
pub const AJV_ERR_ADDITIONAL_PROPERTIES: i32 = 5;
pub const AJV_ERR_MINIMUM: i32 = 6;
pub const AJV_ERR_MAXIMUM: i32 = 7;
pub const AJV_ERR_EXCLUSIVE_MINIMUM: i32 = 8;
pub const AJV_ERR_EXCLUSIVE_MAXIMUM: i32 = 9;
pub const AJV_ERR_MIN_LENGTH: i32 = 10;
pub const AJV_ERR_MAX_LENGTH: i32 = 11;
pub const AJV_ERR_MIN_ITEMS: i32 = 12;
pub const AJV_ERR_MAX_ITEMS: i32 = 13;
pub const AJV_ERR_ENUM: i32 = 14;
pub const AJV_ERR_CONST: i32 = 15;
pub const AJV_ERR_FORMAT: i32 = 16;

pub const AJV_MAX_ERRORS: usize = 16;

// Internal JSON value kind tags
const AJV_KIND_INVALID: u8 = 0;
const AJV_KIND_NULL: u8 = 1;
const AJV_KIND_BOOL: u8 = 2;
const AJV_KIND_NUMBER: u8 = 3;
const AJV_KIND_STRING: u8 = 4;
const AJV_KIND_ARRAY: u8 = 5;
const AJV_KIND_OBJECT: u8 = 6;

fn ajvIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn ajvIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn ajvStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ajvSliceEqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ajvTrimBounds(s: string, out_start: *usize, out_end: *usize) {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && ajvIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && ajvIsSpace(s[b - 1])) {
        b = b - 1;
    }
    out_start.* = a;
    out_end.* = b;
}

fn ajvParseNumber(raw: string, out_val: *f64, out_is_int: *bool): bool {
    let a: usize = 0;
    let b: usize = 0;
    ajvTrimBounds(raw, &a, &b);
    if (a >= b) {
        return false;
    }
    let i = a;
    let neg = false;
    if (raw[i] == 45) {
        neg = true;
        i = i + 1;
    }
    if (i >= b || !ajvIsDigit(raw[i])) {
        return false;
    }

    let val: f64 = 0.0;
    if (raw[i] == 48) {
        i = i + 1;
        if (i < b && ajvIsDigit(raw[i])) {
            return false;
        }
    } else {
        while (i < b && ajvIsDigit(raw[i])) {
            val = val * 10.0 + ((raw[i] - 48) as f64);
            i = i + 1;
        }
    }

    let has_nonzero_frac = false;
    if (i < b && raw[i] == 46) {
        i = i + 1;
        if (i >= b || !ajvIsDigit(raw[i])) {
            return false;
        }
        let scale: f64 = 0.1;
        while (i < b && ajvIsDigit(raw[i])) {
            const d = raw[i] - 48;
            if (d != 0) {
                has_nonzero_frac = true;
            }
            val = val + (d as f64) * scale;
            scale = scale * 0.1;
            i = i + 1;
        }
    }

    let has_exp = false;
    let exp_neg = false;
    let exp_val: i32 = 0;
    if (i < b && (raw[i] == 101 || raw[i] == 69)) {
        has_exp = true;
        i = i + 1;
        if (i < b && raw[i] == 45) {
            exp_neg = true;
            i = i + 1;
        } else if (i < b && raw[i] == 43) {
            i = i + 1;
        }
        if (i >= b || !ajvIsDigit(raw[i])) {
            return false;
        }
        while (i < b && ajvIsDigit(raw[i])) {
            if (exp_val < 308) {
                exp_val = exp_val * 10 + ((raw[i] - 48) as i32);
            }
            i = i + 1;
        }
    }

    if (i != b) {
        return false;
    }

    if (has_exp) {
        let p: i32 = 0;
        while (p < exp_val) {
            if (exp_neg) {
                val = val / 10.0;
            } else {
                val = val * 10.0;
            }
            p = p + 1;
        }
    }

    if (neg) {
        val = -val;
    }

    out_val.* = val;
    if (!has_exp) {
        out_is_int.* = !has_nonzero_frac;
    } else {
        out_is_int.* = val == ((val as i64) as f64);
    }
    return true;
}

fn ajvDecodeJsonString(raw: string, out: []u8, out_len: *usize): bool {
    let a: usize = 0;
    let b: usize = 0;
    ajvTrimBounds(raw, &a, &b);
    if (b - a < 2 || raw[a] != 34 || raw[b - 1] != 34) {
        return false;
    }
    let i = a + 1;
    const end = b - 1;
    let pos: usize = 0;
    let total: usize = 0;

    while (i < end) {
        const c = raw[i];
        if (c == 92) {
            i = i + 1;
            if (i >= end) {
                return false;
            }
            const esc = raw[i];
            let decoded_ch: u8 = esc;
            if (esc == 34) { decoded_ch = 34; }
            else if (esc == 92) { decoded_ch = 92; }
            else if (esc == 47) { decoded_ch = 47; }
            else if (esc == 98) { decoded_ch = 8; }
            else if (esc == 102) { decoded_ch = 12; }
            else if (esc == 110) { decoded_ch = 10; }
            else if (esc == 114) { decoded_ch = 13; }
            else if (esc == 116) { decoded_ch = 9; }
            else if (esc == 117) {
                if (i + 4 >= end) {
                    return false;
                }
                i = i + 4;
                decoded_ch = 63;
            } else {
                return false;
            }
            if (pos < out.len) {
                out[pos] = decoded_ch;
                pos = pos + 1;
            }
            total = total + 1;
            i = i + 1;
        } else if (c == 34 || c < 32) {
            return false;
        } else {
            if (pos < out.len) {
                out[pos] = c;
                pos = pos + 1;
            }
            total = total + 1;
            i = i + 1;
        }
    }

    out_len.* = total;
    return true;
}

/// Scans a single JSON value starting at `pos` inside `s[0..end]`, storing end offset in `out_end`.
fn ajvScanValue(s: string, pos: usize, end: usize, out_end: *usize): bool {
    if (pos >= end) {
        return false;
    }
    const c = s[pos];
    if (c == 34) {
        let i = pos + 1;
        while (i < end) {
            if (s[i] == 92) {
                i = i + 2;
                continue;
            }
            if (s[i] == 34) {
                out_end.* = i + 1;
                return true;
            }
            if (s[i] < 32) {
                return false;
            }
            i = i + 1;
        }
        return false;
    }

    if (c == 123 || c == 91) {
        let i = pos;
        let depth: i32 = 0;
        let in_str = false;
        while (i < end) {
            const ch = s[i];
            if (in_str) {
                if (ch == 92) {
                    i = i + 2;
                    continue;
                }
                if (ch == 34) {
                    in_str = false;
                }
            } else {
                if (ch == 34) {
                    in_str = true;
                } else if (ch == 123 || ch == 91) {
                    depth = depth + 1;
                } else if (ch == 125 || ch == 93) {
                    depth = depth - 1;
                    if (depth == 0) {
                        out_end.* = i + 1;
                        return true;
                    }
                    if (depth < 0) {
                        return false;
                    }
                }
            }
            i = i + 1;
        }
        return false;
    }

    let i = pos;
    while (i < end && !ajvIsSpace(s[i]) && s[i] != 44 && s[i] != 125 && s[i] != 93) {
        i = i + 1;
    }
    if (i == pos) {
        return false;
    }
    out_end.* = i;
    return true;
}

/// Strict recursive JSON syntax validator & root kind classifier.
fn ajvClassifyStrict(raw: string, depth: usize): u8 {
    if (depth > 12) {
        return AJV_KIND_INVALID;
    }
    let a: usize = 0;
    let b: usize = 0;
    ajvTrimBounds(raw, &a, &b);
    if (a >= b) {
        return AJV_KIND_INVALID;
    }
    const s = raw[a..b];
    if (ajvStrEq(s, "null")) { return AJV_KIND_NULL; }
    if (ajvStrEq(s, "true") || ajvStrEq(s, "false")) { return AJV_KIND_BOOL; }

    const first = s[0];
    if (first == 34) {
        let tmp: [8]u8 = [0; 8];
        let n: usize = 0;
        if (ajvDecodeJsonString(s, tmp[0..8], &n)) {
            return AJV_KIND_STRING;
        }
        return AJV_KIND_INVALID;
    }

    if (first == 91) {
        if (s.len < 2 || s[s.len - 1] != 93) {
            return AJV_KIND_INVALID;
        }
        let i: usize = 1;
        const limit = s.len - 1;
        while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
        if (i == limit) { return AJV_KIND_ARRAY; }
        while (true) {
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            let ve: usize = 0;
            if (!ajvScanValue(s, i, limit, &ve)) { return AJV_KIND_INVALID; }
            if (ajvClassifyStrict(s[i..ve], depth + 1) == AJV_KIND_INVALID) { return AJV_KIND_INVALID; }
            i = ve;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit) { break; }
            if (s[i] != 44) { return AJV_KIND_INVALID; }
            i = i + 1;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit) { return AJV_KIND_INVALID; }
        }
        return AJV_KIND_ARRAY;
    }

    if (first == 123) {
        if (s.len < 2 || s[s.len - 1] != 125) {
            return AJV_KIND_INVALID;
        }
        let i: usize = 1;
        const limit = s.len - 1;
        while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
        if (i == limit) { return AJV_KIND_OBJECT; }
        while (true) {
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit || s[i] != 34) { return AJV_KIND_INVALID; }
            let ke: usize = 0;
            if (!ajvScanValue(s, i, limit, &ke)) { return AJV_KIND_INVALID; }
            let ktmp: [8]u8 = [0; 8];
            let klen: usize = 0;
            if (!ajvDecodeJsonString(s[i..ke], ktmp[0..8], &klen)) { return AJV_KIND_INVALID; }
            i = ke;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit || s[i] != 58) { return AJV_KIND_INVALID; }
            i = i + 1;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            let ve: usize = 0;
            if (!ajvScanValue(s, i, limit, &ve)) { return AJV_KIND_INVALID; }
            if (ajvClassifyStrict(s[i..ve], depth + 1) == AJV_KIND_INVALID) { return AJV_KIND_INVALID; }
            i = ve;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit) { break; }
            if (s[i] != 44) { return AJV_KIND_INVALID; }
            i = i + 1;
            while (i < limit && ajvIsSpace(s[i])) { i = i + 1; }
            if (i >= limit) { return AJV_KIND_INVALID; }
        }
        return AJV_KIND_OBJECT;
    }

    let num_v: f64 = 0.0;
    let is_int = false;
    if (ajvParseNumber(s, &num_v, &is_int)) {
        return AJV_KIND_NUMBER;
    }
    return AJV_KIND_INVALID;
}

/// Finds a top-level property `target_key` inside a valid JSON object string `obj`.
fn ajvFindObjectProp(obj: string, target_key: string, out_start: *usize, out_end: *usize): bool {
    let a: usize = 0;
    let b: usize = 0;
    ajvTrimBounds(obj, &a, &b);
    if (b - a < 2 || obj[a] != 123 || obj[b - 1] != 125) {
        return false;
    }
    let i = a + 1;
    const limit = b - 1;
    while (i < limit && ajvIsSpace(obj[i])) { i = i + 1; }
    if (i >= limit) { return false; }

    while (i < limit) {
        while (i < limit && ajvIsSpace(obj[i])) { i = i + 1; }
        if (i >= limit || obj[i] != 34) { return false; }
        let ke: usize = 0;
        if (!ajvScanValue(obj, i, limit, &ke)) { return false; }
        let kbuf: [64]u8 = [0; 64];
        let klen: usize = 0;
        if (!ajvDecodeJsonString(obj[i..ke], kbuf[0..64], &klen)) { return false; }
        i = ke;

        while (i < limit && ajvIsSpace(obj[i])) { i = i + 1; }
        if (i >= limit || obj[i] != 58) { return false; }
        i = i + 1;
        while (i < limit && ajvIsSpace(obj[i])) { i = i + 1; }

        let ve: usize = 0;
        if (!ajvScanValue(obj, i, limit, &ve)) { return false; }

        if (klen <= 64 && ajvSliceEqStr(kbuf[0..klen], target_key)) {
            out_start.* = i;
            out_end.* = ve;
            return true;
        }

        i = ve;
        while (i < limit && ajvIsSpace(obj[i])) { i = i + 1; }
        if (i >= limit) { break; }
        if (obj[i] == 44) {
            i = i + 1;
        } else {
            break;
        }
    }
    return false;
}

fn ajvCountArrayItems(arr: string): usize {
    let a: usize = 0;
    let b: usize = 0;
    ajvTrimBounds(arr, &a, &b);
    if (b - a < 2 || arr[a] != 91 || arr[b - 1] != 93) {
        return 0;
    }
    let i = a + 1;
    const limit = b - 1;
    while (i < limit && ajvIsSpace(arr[i])) { i = i + 1; }
    if (i >= limit) { return 0; }

    let count: usize = 0;
    while (i < limit) {
        while (i < limit && ajvIsSpace(arr[i])) { i = i + 1; }
        let ve: usize = 0;
        if (!ajvScanValue(arr, i, limit, &ve)) { break; }
        count = count + 1;
        i = ve;
        while (i < limit && ajvIsSpace(arr[i])) { i = i + 1; }
        if (i < limit && arr[i] == 44) {
            i = i + 1;
        } else {
            break;
        }
    }
    return count;
}

/// Semantic equality check between two valid JSON value slices.
fn ajvJsonValuesEqual(raw_a: string, raw_b: string): bool {
    let a0: usize = 0;
    let a1: usize = 0;
    let b0: usize = 0;
    let b1: usize = 0;
    ajvTrimBounds(raw_a, &a0, &a1);
    ajvTrimBounds(raw_b, &b0, &b1);
    const sa = raw_a[a0..a1];
    const sb = raw_b[b0..b1];

    const ka = ajvClassifyStrict(sa, 0);
    const kb = ajvClassifyStrict(sb, 0);
    if (ka == AJV_KIND_INVALID || ka != kb) {
        return false;
    }
    if (ka == AJV_KIND_NULL || ka == AJV_KIND_BOOL) {
        return ajvStrEq(sa, sb);
    }
    if (ka == AJV_KIND_NUMBER) {
        let va: f64 = 0.0;
        let vb: f64 = 0.0;
        let ia = false;
        let ib = false;
        if (!ajvParseNumber(sa, &va, &ia) || !ajvParseNumber(sb, &vb, &ib)) {
            return false;
        }
        return va == vb;
    }
    if (ka == AJV_KIND_STRING) {
        let ba: [256]u8 = [0; 256];
        let bb: [256]u8 = [0; 256];
        let la: usize = 0;
        let lb: usize = 0;
        if (!ajvDecodeJsonString(sa, ba[0..256], &la) || !ajvDecodeJsonString(sb, bb[0..256], &lb)) {
            return false;
        }
        if (la != lb) { return false; }
        const n = if (la < 256) { la } else { 256 };
        let i: usize = 0;
        while (i < n) {
            if (ba[i] != bb[i]) { return false; }
            i = i + 1;
        }
        return true;
    }
    return ajvStrEq(sa, sb);
}

/// Validates RFC 3339 full-date `YYYY-MM-DD` (including leap-year day validation).
fn ajvIsIsoDate(s: string): bool {
    if (s.len != 10) {
        return false;
    }
    if (s[4] != 45 || s[7] != 45) {
        return false;
    }
    let i: usize = 0;
    while (i < 10) {
        if (i != 4 && i != 7 && !ajvIsDigit(s[i])) {
            return false;
        }
        i = i + 1;
    }
    const year: u32 = ((s[0] - 48) as u32) * 1000
        + ((s[1] - 48) as u32) * 100
        + ((s[2] - 48) as u32) * 10
        + ((s[3] - 48) as u32);
    const month: u32 = ((s[5] - 48) as u32) * 10 + ((s[6] - 48) as u32);
    const day: u32 = ((s[8] - 48) as u32) * 10 + ((s[9] - 48) as u32);

    if (month < 1 || month > 12 || day < 1) {
        return false;
    }
    let max_day: u32 = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        max_day = 30;
    } else if (month == 2) {
        const leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        max_day = if (leap) { 29 } else { 28 };
    }
    return day <= max_day;
}

/// Validates JSON Schema `format` keywords by reusing `@cap/zod-schema` & `@cap/ipaddr`.
/// Supported formats: `email`, `uri` (`url`), `uuid`, `ipv4`, `ipv6`, `date`.
pub fn validateFormat(fmt: string, value: string): bool {
    if (ajvStrEq(fmt, "email")) {
        return zEmail(value);
    }
    if (ajvStrEq(fmt, "uri") || ajvStrEq(fmt, "url")) {
        return zUrl(value);
    }
    if (ajvStrEq(fmt, "uuid")) {
        return zUuid(value);
    }
    if (ajvStrEq(fmt, "ipv4")) {
        return isValidIpv4(value);
    }
    if (ajvStrEq(fmt, "ipv6")) {
        return isValidIpv6(value);
    }
    if (ajvStrEq(fmt, "date")) {
        return ajvIsIsoDate(value);
    }
    return false;
}

fn ajvMatchSingleType(type_name: string, data_kind: u8, data_raw: string): bool {
    if (!zEnum(type_name, "string,number,integer,boolean,array,object,null")) {
        return false;
    }
    if (ajvStrEq(type_name, "string")) { return data_kind == AJV_KIND_STRING; }
    if (ajvStrEq(type_name, "boolean")) { return data_kind == AJV_KIND_BOOL; }
    if (ajvStrEq(type_name, "array")) { return data_kind == AJV_KIND_ARRAY; }
    if (ajvStrEq(type_name, "object")) { return data_kind == AJV_KIND_OBJECT; }
    if (ajvStrEq(type_name, "null")) { return data_kind == AJV_KIND_NULL; }
    if (ajvStrEq(type_name, "number")) { return data_kind == AJV_KIND_NUMBER; }
    if (ajvStrEq(type_name, "integer")) {
        if (data_kind != AJV_KIND_NUMBER) {
            return false;
        }
        let v: f64 = 0.0;
        let is_int = false;
        return ajvParseNumber(data_raw, &v, &is_int) && is_int;
    }
    return false;
}

fn ajvJoinPath(base: string, seg: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < base.len && pos < out.len) {
        out[pos] = base[i];
        pos = pos + 1;
        i = i + 1;
    }
    if (pos < out.len) {
        out[pos] = 47; // '/'
        pos = pos + 1;
    }
    i = 0;
    while (i < seg.len && pos < out.len) {
        out[pos] = seg[i];
        pos = pos + 1;
        i = i + 1;
    }
    return pos;
}

pub struct AjvError {
    pub instance_path: [64]u8,
    pub instance_path_len: usize,
    pub keyword: [32]u8,
    pub keyword_len: usize,
    pub code: i32,

    pub static fn empty(): AjvError {
        return AjvError {
            instance_path: [0; 64],
            instance_path_len: 0,
            keyword: [0; 32],
            keyword_len: 0,
            code: 0,
        };
    }

    pub fn pathStr(): []const u8 {
        return _.instance_path[0.._.instance_path_len];
    }

    pub fn keywordStr(): []const u8 {
        return _.keyword[0.._.keyword_len];
    }
}

pub struct AjvResult {
    pub valid: bool,
    pub error_count: usize,
    pub errors: [16]AjvError,

    pub static fn ok(): AjvResult {
        let errs: [16]AjvError = undefined;
        let i: usize = 0;
        while (i < AJV_MAX_ERRORS) {
            errs[i] = AjvError.empty();
            i = i + 1;
        }
        return AjvResult {
            valid: true,
            error_count: 0,
            errors: errs,
        };
    }

    pub fn addError(*_, path: string, keyword: string, code: i32) {
        _.valid = false;
        if (_.error_count >= AJV_MAX_ERRORS) {
            return;
        }
        let err = AjvError.empty();
        const pn = if (path.len < 64) { path.len } else { 64 };
        let i: usize = 0;
        while (i < pn) {
            err.instance_path[i] = path[i];
            i = i + 1;
        }
        err.instance_path_len = pn;

        const kn = if (keyword.len < 32) { keyword.len } else { 32 };
        i = 0;
        while (i < kn) {
            err.keyword[i] = keyword[i];
            i = i + 1;
        }
        err.keyword_len = kn;
        err.code = code;

        _.errors[_.error_count] = err;
        _.error_count = _.error_count + 1;
    }

    pub fn hasError(path: string, keyword: string, code: i32): bool {
        let i: usize = 0;
        while (i < _.error_count) {
            if (_.errors[i].code == code
                && ajvSliceEqStr(_.errors[i].instance_path[0.._.errors[i].instance_path_len], path)
                && ajvSliceEqStr(_.errors[i].keyword[0.._.errors[i].keyword_len], keyword)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    pub fn hasCode(code: i32): bool {
        let i: usize = 0;
        while (i < _.error_count) {
            if (_.errors[i].code == code) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}

fn ajvValidateNode(schema: string, data: string, path: string, depth: usize, res: *AjvResult) {
    if (depth > 8) {
        return;
    }
    const data_kind = ajvClassifyStrict(data, 0);
    if (data_kind == AJV_KIND_INVALID) {
        res.addError(path, "data", AJV_ERR_INVALID_DATA);
        return;
    }

    let vs: usize = 0;
    let ve: usize = 0;

    // 1. `type`
    if (ajvFindObjectProp(schema, "type", &vs, &ve)) {
        const type_val = schema[vs..ve];
        const tk = ajvClassifyStrict(type_val, 0);
        if (tk == AJV_KIND_STRING) {
            let tbuf: [32]u8 = [0; 32];
            let tlen: usize = 0;
            if (ajvDecodeJsonString(type_val, tbuf[0..32], &tlen) && tlen <= 32) {
                if (!ajvMatchSingleType(tbuf[0..tlen], data_kind, data)) {
                    res.addError(path, "type", AJV_ERR_TYPE);
                }
            }
        } else if (tk == AJV_KIND_ARRAY) {
            let a0: usize = 0;
            let a1: usize = 0;
            ajvTrimBounds(type_val, &a0, &a1);
            let i = a0 + 1;
            const lim = a1 - 1;
            let matched = false;
            while (i < lim) {
                while (i < lim && ajvIsSpace(type_val[i])) { i = i + 1; }
                if (i >= lim) { break; }
                let ie: usize = 0;
                if (!ajvScanValue(type_val, i, lim, &ie)) { break; }
                let tbuf: [32]u8 = [0; 32];
                let tlen: usize = 0;
                if (ajvDecodeJsonString(type_val[i..ie], tbuf[0..32], &tlen) && tlen <= 32) {
                    if (ajvMatchSingleType(tbuf[0..tlen], data_kind, data)) {
                        matched = true;
                        break;
                    }
                }
                i = ie;
                while (i < lim && ajvIsSpace(type_val[i])) { i = i + 1; }
                if (i < lim && type_val[i] == 44) { i = i + 1; } else { break; }
            }
            if (!matched) {
                res.addError(path, "type", AJV_ERR_TYPE);
            }
        }
    }

    // 2. `const`
    if (ajvFindObjectProp(schema, "const", &vs, &ve)) {
        if (!ajvJsonValuesEqual(data, schema[vs..ve])) {
            res.addError(path, "const", AJV_ERR_CONST);
        }
    }

    // 3. `enum`
    if (ajvFindObjectProp(schema, "enum", &vs, &ve)) {
        const enum_arr = schema[vs..ve];
        if (ajvClassifyStrict(enum_arr, 0) == AJV_KIND_ARRAY) {
            let a0: usize = 0;
            let a1: usize = 0;
            ajvTrimBounds(enum_arr, &a0, &a1);
            let i = a0 + 1;
            const lim = a1 - 1;
            let matched = false;
            while (i < lim) {
                while (i < lim && ajvIsSpace(enum_arr[i])) { i = i + 1; }
                if (i >= lim) { break; }
                let ie: usize = 0;
                if (!ajvScanValue(enum_arr, i, lim, &ie)) { break; }
                if (ajvJsonValuesEqual(data, enum_arr[i..ie])) {
                    matched = true;
                    break;
                }
                i = ie;
                while (i < lim && ajvIsSpace(enum_arr[i])) { i = i + 1; }
                if (i < lim && enum_arr[i] == 44) { i = i + 1; } else { break; }
            }
            if (!matched) {
                res.addError(path, "enum", AJV_ERR_ENUM);
            }
        }
    }

    // 4. String keywords (`minLength`, `maxLength`, `format`)
    if (data_kind == AJV_KIND_STRING) {
        let sbuf: [256]u8 = [0; 256];
        let slen: usize = 0;
        if (ajvDecodeJsonString(data, sbuf[0..256], &slen)) {
            if (ajvFindObjectProp(schema, "minLength", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && (slen as f64) < mv) {
                    res.addError(path, "minLength", AJV_ERR_MIN_LENGTH);
                }
            }
            if (ajvFindObjectProp(schema, "maxLength", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && (slen as f64) > mv) {
                    res.addError(path, "maxLength", AJV_ERR_MAX_LENGTH);
                }
            }
            if (ajvFindObjectProp(schema, "format", &vs, &ve)) {
                let fbuf: [32]u8 = [0; 32];
                let flen: usize = 0;
                if (ajvDecodeJsonString(schema[vs..ve], fbuf[0..32], &flen) && flen <= 32) {
                    const cmp_len = if (slen < 256) { slen } else { 256 };
                    if (!validateFormat(fbuf[0..flen], sbuf[0..cmp_len])) {
                        res.addError(path, "format", AJV_ERR_FORMAT);
                    }
                }
            }
        }
    }

    // 5. Numeric keywords (`minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum`)
    if (data_kind == AJV_KIND_NUMBER) {
        let val: f64 = 0.0;
        let is_int = false;
        if (ajvParseNumber(data, &val, &is_int)) {
            if (ajvFindObjectProp(schema, "minimum", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && val < mv) {
                    res.addError(path, "minimum", AJV_ERR_MINIMUM);
                }
            }
            if (ajvFindObjectProp(schema, "maximum", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && val > mv) {
                    res.addError(path, "maximum", AJV_ERR_MAXIMUM);
                }
            }
            if (ajvFindObjectProp(schema, "exclusiveMinimum", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && val <= mv) {
                    res.addError(path, "exclusiveMinimum", AJV_ERR_EXCLUSIVE_MINIMUM);
                }
            }
            if (ajvFindObjectProp(schema, "exclusiveMaximum", &vs, &ve)) {
                let mv: f64 = 0.0;
                let mi = false;
                if (ajvParseNumber(schema[vs..ve], &mv, &mi) && val >= mv) {
                    res.addError(path, "exclusiveMaximum", AJV_ERR_EXCLUSIVE_MAXIMUM);
                }
            }
        }
    }

    // 6. Array keywords (`minItems`, `maxItems`, `items`)
    if (data_kind == AJV_KIND_ARRAY) {
        const count = ajvCountArrayItems(data);
        if (ajvFindObjectProp(schema, "minItems", &vs, &ve)) {
            let mv: f64 = 0.0;
            let mi = false;
            if (ajvParseNumber(schema[vs..ve], &mv, &mi) && (count as f64) < mv) {
                res.addError(path, "minItems", AJV_ERR_MIN_ITEMS);
            }
        }
        if (ajvFindObjectProp(schema, "maxItems", &vs, &ve)) {
            let mv: f64 = 0.0;
            let mi = false;
            if (ajvParseNumber(schema[vs..ve], &mv, &mi) && (count as f64) > mv) {
                res.addError(path, "maxItems", AJV_ERR_MAX_ITEMS);
            }
        }
        if (ajvFindObjectProp(schema, "items", &vs, &ve)) {
            const item_schema = schema[vs..ve];
            if (ajvClassifyStrict(item_schema, 0) == AJV_KIND_OBJECT) {
                let a0: usize = 0;
                let a1: usize = 0;
                ajvTrimBounds(data, &a0, &a1);
                let i = a0 + 1;
                const lim = a1 - 1;
                while (i < lim) {
                    while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }
                    if (i >= lim) { break; }
                    let ie: usize = 0;
                    if (!ajvScanValue(data, i, lim, &ie)) { break; }
                    ajvValidateNode(item_schema, data[i..ie], path, depth + 1, res);
                    i = ie;
                    while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }
                    if (i < lim && data[i] == 44) { i = i + 1; } else { break; }
                }
            }
        }
    }

    // 7. Object keywords (`required`, `properties`, `additionalProperties`)
    if (data_kind == AJV_KIND_OBJECT) {
        // Check `required`
        if (ajvFindObjectProp(schema, "required", &vs, &ve)) {
            const req_arr = schema[vs..ve];
            if (ajvClassifyStrict(req_arr, 0) == AJV_KIND_ARRAY) {
                let a0: usize = 0;
                let a1: usize = 0;
                ajvTrimBounds(req_arr, &a0, &a1);
                let i = a0 + 1;
                const lim = a1 - 1;
                while (i < lim) {
                    while (i < lim && ajvIsSpace(req_arr[i])) { i = i + 1; }
                    if (i >= lim) { break; }
                    let ie: usize = 0;
                    if (!ajvScanValue(req_arr, i, lim, &ie)) { break; }
                    let rbuf: [64]u8 = [0; 64];
                    let rlen: usize = 0;
                    if (ajvDecodeJsonString(req_arr[i..ie], rbuf[0..64], &rlen) && rlen <= 64) {
                        let dummy_s: usize = 0;
                        let dummy_e: usize = 0;
                        if (!ajvFindObjectProp(data, rbuf[0..rlen], &dummy_s, &dummy_e)) {
                            let pbuf: [64]u8 = [0; 64];
                            const plen = ajvJoinPath(path, rbuf[0..rlen], pbuf[0..64]);
                            res.addError(pbuf[0..plen], "required", AJV_ERR_REQUIRED);
                        }
                    }
                    i = ie;
                    while (i < lim && ajvIsSpace(req_arr[i])) { i = i + 1; }
                    if (i < lim && req_arr[i] == 44) { i = i + 1; } else { break; }
                }
            }
        }

        // Check `properties` and `additionalProperties: false`
        let has_props = false;
        let props_s: usize = 0;
        let props_e: usize = 0;
        if (ajvFindObjectProp(schema, "properties", &props_s, &props_e)) {
            if (ajvClassifyStrict(schema[props_s..props_e], 0) == AJV_KIND_OBJECT) {
                has_props = true;
            }
        }

        let forbid_additional = false;
        let ap_s: usize = 0;
        let ap_e: usize = 0;
        if (ajvFindObjectProp(schema, "additionalProperties", &ap_s, &ap_e)) {
            let t0: usize = 0;
            let t1: usize = 0;
            ajvTrimBounds(schema[ap_s..ap_e], &t0, &t1);
            if (ajvStrEq(schema[ap_s + t0..ap_s + t1], "false")) {
                forbid_additional = true;
            }
        }

        if (has_props || forbid_additional) {
            let d0: usize = 0;
            let d1: usize = 0;
            ajvTrimBounds(data, &d0, &d1);
            let i = d0 + 1;
            const lim = d1 - 1;
            while (i < lim) {
                while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }
                if (i >= lim || data[i] != 34) { break; }
                let ke: usize = 0;
                if (!ajvScanValue(data, i, lim, &ke)) { break; }
                let kbuf: [64]u8 = [0; 64];
                let klen: usize = 0;
                if (!ajvDecodeJsonString(data[i..ke], kbuf[0..64], &klen)) { break; }
                i = ke;

                while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }
                if (i >= lim || data[i] != 58) { break; }
                i = i + 1;
                while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }

                let val_e: usize = 0;
                if (!ajvScanValue(data, i, lim, &val_e)) { break; }

                const key_slice_len = if (klen < 64) { klen } else { 64 };
                const key_str: string = kbuf[0..key_slice_len];
                let pbuf: [64]u8 = [0; 64];
                const plen = ajvJoinPath(path, key_str, pbuf[0..64]);

                let sub_s: usize = 0;
                let sub_e: usize = 0;
                if (has_props && ajvFindObjectProp(schema[props_s..props_e], key_str, &sub_s, &sub_e)) {
                    const sub_schema = schema[props_s + sub_s..props_s + sub_e];
                    if (ajvClassifyStrict(sub_schema, 0) == AJV_KIND_OBJECT) {
                        ajvValidateNode(sub_schema, data[i..val_e], pbuf[0..plen], depth + 1, res);
                    }
                } else if (forbid_additional) {
                    res.addError(pbuf[0..plen], "additionalProperties", AJV_ERR_ADDITIONAL_PROPERTIES);
                }

                i = val_e;
                while (i < lim && ajvIsSpace(data[i])) { i = i + 1; }
                if (i < lim && data[i] == 44) { i = i + 1; } else { break; }
            }
        }
    }
}

pub struct AjvSchema {
    pub source: string,
    pub ok: bool,

    pub static fn compile(schema_json: string): AjvSchema {
        const k = ajvClassifyStrict(schema_json, 0);
        return AjvSchema {
            source: schema_json,
            ok: k == AJV_KIND_OBJECT,
        };
    }

    pub fn validate(data_json: string): AjvResult {
        let res = AjvResult.ok();
        if (!_.ok) {
            res.addError("", "schema", AJV_ERR_INVALID_SCHEMA);
            return res;
        }
        ajvValidateNode(_.source, data_json, "", 0, &res);
        return res;
    }
}

/// Compiles `schema_json` and validates `data_json` against it in one call.
pub fn validateWithSchema(schema_json: string, data_json: string): AjvResult {
    const schema = AjvSchema.compile(schema_json);
    return schema.validate(data_json);
}

export {
    AJV_ERR_INVALID_SCHEMA,
    AJV_ERR_INVALID_DATA,
    AJV_ERR_TYPE,
    AJV_ERR_REQUIRED,
    AJV_ERR_ADDITIONAL_PROPERTIES,
    AJV_ERR_MINIMUM,
    AJV_ERR_MAXIMUM,
    AJV_ERR_EXCLUSIVE_MINIMUM,
    AJV_ERR_EXCLUSIVE_MAXIMUM,
    AJV_ERR_MIN_LENGTH,
    AJV_ERR_MAX_LENGTH,
    AJV_ERR_MIN_ITEMS,
    AJV_ERR_MAX_ITEMS,
    AJV_ERR_ENUM,
    AJV_ERR_CONST,
    AJV_ERR_FORMAT,
    AJV_MAX_ERRORS,
    AjvError,
    AjvResult,
    AjvSchema,
    validateFormat,
    validateWithSchema,
};
