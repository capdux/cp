// @cap/zod-schema — Zero-heap type-safe data validation framework (zod v3 parity)
// Clean-room MIT CP implementation of npm `zod` (150M+/wk).
// Reuses:
//   - `kindOf`, `KIND_*` from `@cap/kind-of`
//   - `WhatwgUrl` (`WhatwgUrl.parse`) from `@cap/whatwg-url`

import {
    kindOf,
    KIND_INVALID,
    KIND_NULL,
    KIND_UNDEFINED,
    KIND_BOOLEAN,
    KIND_NUMBER,
    KIND_STRING,
    KIND_ARRAY,
    KIND_OBJECT,
    KIND_DATE,
} from "../../kind-of/src/index.cp";
import { WhatwgUrl } from "../../url/src/index.cp";

// ZRuleKind constants
pub const Z_STRING: u8 = 1;
pub const Z_NUMBER: u8 = 2;
pub const Z_INT: u8 = 3;
pub const Z_BOOL: u8 = 4;
pub const Z_EMAIL: u8 = 5;
pub const Z_URL: u8 = 6;
pub const Z_UUID: u8 = 7;
pub const Z_ENUM: u8 = 8;
pub const Z_ARRAY: u8 = 9;
pub const Z_OBJECT: u8 = 10;

// ZIssue error codes
pub const Z_ERR_INVALID_JSON: i32 = 1;
pub const Z_ERR_NOT_OBJECT: i32 = 2;
pub const Z_ERR_REQUIRED: i32 = 3;
pub const Z_ERR_INVALID_TYPE: i32 = 4;
pub const Z_ERR_NOT_INT: i32 = 5;
pub const Z_ERR_TOO_SMALL: i32 = 6;
pub const Z_ERR_TOO_BIG: i32 = 7;
pub const Z_ERR_INVALID_EMAIL: i32 = 8;
pub const Z_ERR_INVALID_URL: i32 = 9;
pub const Z_ERR_INVALID_UUID: i32 = 10;
pub const Z_ERR_INVALID_ENUM: i32 = 11;

pub const Z_MAX_RULES: usize = 24;
pub const Z_MAX_ISSUES: usize = 16;

fn zdIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn zdIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn zdIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn zdIsHex(c: u8): bool {
    return zdIsDigit(c) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
}

fn zdIsEmailLocalChar(c: u8): bool {
    if (zdIsAlpha(c) || zdIsDigit(c)) {
        return true;
    }
    return c == 46 || c == 95 || c == 45 || c == 43 || c == 37;
}

fn zdIsDomainLabelChar(c: u8): bool {
    return zdIsAlpha(c) || zdIsDigit(c) || c == 45;
}

fn zdStrEq(a: string, b: string): bool {
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

fn zdSliceEqStr(a: []const u8, b: string): bool {
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

fn zdTrimBounds(s: string, out_start: *usize, out_end: *usize) {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && zdIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && zdIsSpace(s[b - 1])) {
        b = b - 1;
    }
    out_start.* = a;
    out_end.* = b;
}

/// Parses a finite ASCII decimal/scientific float into `out_val`.
/// Also sets `out_is_int.*` to true if the value represents an exact integer.
fn zdParseNumber(raw: string, out_val: *f64, out_is_int: *bool): bool {
    let a: usize = 0;
    let b: usize = 0;
    zdTrimBounds(raw, &a, &b);
    if (a >= b) {
        return false;
    }

    let i = a;
    let neg = false;
    if (raw[i] == 45) {
        neg = true;
        i = i + 1;
    } else if (raw[i] == 43) {
        i = i + 1;
    }
    if (i >= b) {
        return false;
    }

    let val: f64 = 0.0;
    let has_digits = false;
    while (i < b && zdIsDigit(raw[i])) {
        has_digits = true;
        val = val * 10.0 + ((raw[i] - 48) as f64);
        i = i + 1;
    }

    let has_nonzero_frac = false;
    if (i < b && raw[i] == 46) {
        i = i + 1;
        let scale: f64 = 0.1;
        let frac_digits = false;
        while (i < b && zdIsDigit(raw[i])) {
            has_digits = true;
            frac_digits = true;
            const d = raw[i] - 48;
            if (d != 0) {
                has_nonzero_frac = true;
            }
            val = val + (d as f64) * scale;
            scale = scale * 0.1;
            i = i + 1;
        }
        if (!frac_digits) {
            return false;
        }
    }

    if (!has_digits) {
        return false;
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
        let exp_digits = false;
        while (i < b && zdIsDigit(raw[i])) {
            exp_digits = true;
            if (exp_val < 308) {
                exp_val = exp_val * 10 + ((raw[i] - 48) as i32);
            }
            i = i + 1;
        }
        if (!exp_digits) {
            return false;
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
        const trunc = (val as i64) as f64;
        out_is_int.* = val == trunc;
    }
    return true;
}

/// Decodes a JSON double-quoted string literal `raw` (`"..."`) into `out`,
/// returning decoded length (or -1 if invalid JSON string syntax).
fn zdDecodeJsonString(raw: string, out: []u8, out_total_len: *usize): bool {
    if (raw.len < 2 || raw[0] != 34 || raw[raw.len - 1] != 34) {
        return false;
    }
    let i: usize = 1;
    const end = raw.len - 1;
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
            let decoded: u8 = esc;
            if (esc == 34) { decoded = 34; }
            else if (esc == 92) { decoded = 92; }
            else if (esc == 47) { decoded = 47; }
            else if (esc == 98) { decoded = 8; }
            else if (esc == 102) { decoded = 12; }
            else if (esc == 110) { decoded = 10; }
            else if (esc == 114) { decoded = 13; }
            else if (esc == 116) { decoded = 9; }
            else if (esc == 117) {
                if (i + 4 >= end) {
                    return false;
                }
                i = i + 4;
                decoded = 63; // '?'
            } else {
                return false;
            }
            if (pos < out.len) {
                out[pos] = decoded;
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

    out_total_len.* = total;
    return true;
}

/// Counts top-level elements in a JSON array `[ ... ]`.
fn zdCountJsonArrayItems(raw: string, out_count: *usize): bool {
    let a: usize = 0;
    let b: usize = 0;
    zdTrimBounds(raw, &a, &b);
    if (b - a < 2 || raw[a] != 91 || raw[b - 1] != 93) {
        return false;
    }
    let i = a + 1;
    const end = b - 1;
    while (i < end && zdIsSpace(raw[i])) {
        i = i + 1;
    }
    if (i == end) {
        out_count.* = 0;
        return true;
    }

    let count: usize = 1;
    let depth: i32 = 0;
    let in_str = false;
    while (i < end) {
        const c = raw[i];
        if (in_str) {
            if (c == 92) {
                i = i + 2;
                continue;
            }
            if (c == 34) {
                in_str = false;
            }
        } else {
            if (c == 34) {
                in_str = true;
            } else if (c == 91 || c == 123) {
                depth = depth + 1;
            } else if (c == 93 || c == 125) {
                depth = depth - 1;
                if (depth < 0) {
                    return false;
                }
            } else if (c == 44 && depth == 0) {
                count = count + 1;
            }
        }
        i = i + 1;
    }

    if (in_str || depth != 0) {
        return false;
    }
    out_count.* = count;
    return true;
}

/// Validates a string (plain or JSON-quoted) against `[min_len, max_len]`.
pub fn zString(raw: string, min_len: usize, max_len: usize): bool {
    let slen = raw.len;
    if (raw.len >= 2 && raw[0] == 34 && raw[raw.len - 1] == 34) {
        let tmp: [8]u8 = [0; 8];
        let decoded_len: usize = 0;
        if (!zdDecodeJsonString(raw, tmp[0..8], &decoded_len)) {
            return false;
        }
        slen = decoded_len;
    }
    return slen >= min_len && slen <= max_len;
}

/// Validates a numeric string using `@cap/kind-of` (`KIND_NUMBER`) and `[min_v, max_v]` bounds.
pub fn zNumber(raw: string, min_v: f64, max_v: f64): bool {
    if (kindOf(raw) != KIND_NUMBER) {
        return false;
    }
    let val: f64 = 0.0;
    let is_int = false;
    if (!zdParseNumber(raw, &val, &is_int)) {
        return false;
    }
    return val >= min_v && val <= max_v;
}

/// Validates an ASCII email address (`local@domain.tld`).
pub fn zEmail(s: string): bool {
    if (s.len < 5 || s.len > 254) {
        return false;
    }
    let at_pos: usize = 0;
    let at_count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 64) {
            at_pos = i;
            at_count = at_count + 1;
        } else if (zdIsSpace(s[i])) {
            return false;
        }
        i = i + 1;
    }
    if (at_count != 1 || at_pos == 0 || at_pos + 3 >= s.len || at_pos > 64) {
        return false;
    }

    // Validate local part
    if (s[0] == 46 || s[at_pos - 1] == 46) {
        return false;
    }
    i = 0;
    while (i < at_pos) {
        if (!zdIsEmailLocalChar(s[i])) {
            return false;
        }
        if (s[i] == 46 && i + 1 < at_pos && s[i + 1] == 46) {
            return false;
        }
        i = i + 1;
    }

    // Validate domain part
    const dom_start = at_pos + 1;
    if (s[dom_start] == 46 || s[s.len - 1] == 46) {
        return false;
    }
    let dot_count: usize = 0;
    let label_len: usize = 0;
    let last_label_start = dom_start;
    i = dom_start;
    while (i < s.len) {
        const c = s[i];
        if (c == 46) {
            if (label_len == 0 || s[i - 1] == 45) {
                return false;
            }
            dot_count = dot_count + 1;
            label_len = 0;
            last_label_start = i + 1;
        } else {
            if (!zdIsDomainLabelChar(c)) {
                return false;
            }
            if (label_len == 0 && c == 45) {
                return false;
            }
            label_len = label_len + 1;
        }
        i = i + 1;
    }
    if (dot_count == 0 || label_len < 2 || s[s.len - 1] == 45) {
        return false;
    }

    // TLD must be alphabetic
    i = last_label_start;
    while (i < s.len) {
        if (!zdIsAlpha(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Validates an absolute URL by delegating directly to `@cap/whatwg-url` (`WhatwgUrl.parse`).
pub fn zUrl(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    const parsed = WhatwgUrl.parse(s);
    return parsed.ok && parsed.scheme_len > 0;
}

/// Validates a canonical 36-character UUID (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).
pub fn zUuid(s: string): bool {
    if (s.len != 36) {
        return false;
    }
    let i: usize = 0;
    while (i < 36) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (s[i] != 45) {
                return false;
            }
        } else {
            if (!zdIsHex(s[i])) {
                return false;
            }
        }
        i = i + 1;
    }
    return true;
}

/// Validates that `s` matches one of the comma-separated values in `csv` (e.g. `"admin,user,guest"`).
pub fn zEnum(s: string, csv: string): bool {
    if (csv.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i <= csv.len) {
        const start = i;
        while (i < csv.len && csv[i] != 44) {
            i = i + 1;
        }
        if (zdStrEq(s, csv[start..i])) {
            return true;
        }
        if (i >= csv.len) {
            break;
        }
        i = i + 1;
    }
    return false;
}

pub struct ZFieldRule {
    pub name: string,
    pub kind: u8,
    pub required: bool,
    pub min_val: f64,
    pub max_val: f64,
    pub has_min: bool,
    pub has_max: bool,
    pub min_len: usize,
    pub max_len: usize,
    pub has_min_len: bool,
    pub has_max_len: bool,
    pub enum_csv: string,

    pub static fn empty(): ZFieldRule {
        return ZFieldRule {
            name: "",
            kind: Z_STRING,
            required: false,
            min_val: 0.0,
            max_val: 0.0,
            has_min: false,
            has_max: false,
            min_len: 0,
            max_len: 0,
            has_min_len: false,
            has_max_len: false,
            enum_csv: "",
        };
    }

    pub static fn basic(name: string, kind: u8, required: bool): ZFieldRule {
        return ZFieldRule {
            name: name,
            kind: kind,
            required: required,
            min_val: 0.0,
            max_val: 0.0,
            has_min: false,
            has_max: false,
            min_len: 0,
            max_len: 0,
            has_min_len: false,
            has_max_len: false,
            enum_csv: "",
        };
    }

    pub static fn strRange(name: string, required: bool, min_len: usize, max_len: usize): ZFieldRule {
        return ZFieldRule {
            name: name,
            kind: Z_STRING,
            required: required,
            min_val: 0.0,
            max_val: 0.0,
            has_min: false,
            has_max: false,
            min_len: min_len,
            max_len: max_len,
            has_min_len: true,
            has_max_len: true,
            enum_csv: "",
        };
    }

    pub static fn numRange(name: string, kind: u8, required: bool, min_val: f64, max_val: f64): ZFieldRule {
        return ZFieldRule {
            name: name,
            kind: kind,
            required: required,
            min_val: min_val,
            max_val: max_val,
            has_min: true,
            has_max: true,
            min_len: 0,
            max_len: 0,
            has_min_len: false,
            has_max_len: false,
            enum_csv: "",
        };
    }

    pub static fn enumRule(name: string, required: bool, enum_csv: string): ZFieldRule {
        return ZFieldRule {
            name: name,
            kind: Z_ENUM,
            required: required,
            min_val: 0.0,
            max_val: 0.0,
            has_min: false,
            has_max: false,
            min_len: 0,
            max_len: 0,
            has_min_len: false,
            has_max_len: false,
            enum_csv: enum_csv,
        };
    }
}

pub struct ZIssue {
    pub field: [48]u8,
    pub field_len: usize,
    pub code: i32,

    pub static fn empty(): ZIssue {
        return ZIssue {
            field: [0; 48],
            field_len: 0,
            code: 0,
        };
    }

    pub fn fieldName(): []const u8 {
        return _.field[0.._.field_len];
    }
}

pub struct ZParseResult {
    pub ok: bool,
    pub issue_count: usize,
    pub issues: [16]ZIssue,

    pub static fn empty(): ZParseResult {
        let issues: [16]ZIssue = undefined;
        let i: usize = 0;
        while (i < 16) {
            issues[i] = ZIssue.empty();
            i = i + 1;
        }
        return ZParseResult {
            ok: true,
            issue_count: 0,
            issues: issues,
        };
    }

    pub fn addIssue(*_, field_name: string, code: i32) {
        _.ok = false;
        if (_.issue_count >= Z_MAX_ISSUES) {
            return;
        }
        let issue = ZIssue.empty();
        const n = if (field_name.len < 48) { field_name.len } else { 48 };
        let i: usize = 0;
        while (i < n) {
            issue.field[i] = field_name[i];
            i = i + 1;
        }
        issue.field_len = n;
        issue.code = code;
        _.issues[_.issue_count] = issue;
        _.issue_count = _.issue_count + 1;
    }

    pub fn hasIssue(field_name: string, code: i32): bool {
        let i: usize = 0;
        while (i < _.issue_count) {
            if (_.issues[i].code == code && zdSliceEqStr(_.issues[i].field[0.._.issues[i].field_len], field_name)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }
}

fn zdValidateFieldValue(rule: ZFieldRule, raw_val: string, res: *ZParseResult) {
    const k = kindOf(raw_val);
    if (k == KIND_NULL || k == KIND_UNDEFINED) {
        if (rule.required) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
        }
        return;
    }

    if (rule.kind == Z_STRING || rule.kind == Z_EMAIL || rule.kind == Z_URL || rule.kind == Z_UUID || rule.kind == Z_ENUM) {
        if ((k != KIND_STRING && k != KIND_DATE) || raw_val.len < 2 || raw_val[0] != 34 || raw_val[raw_val.len - 1] != 34) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
            return;
        }
        let decoded: [256]u8 = [0; 256];
        let decoded_len: usize = 0;
        if (!zdDecodeJsonString(raw_val, decoded[0..256], &decoded_len)) {
            res.addIssue(rule.name, Z_ERR_INVALID_JSON);
            return;
        }

        if (rule.has_min_len && decoded_len < rule.min_len) {
            res.addIssue(rule.name, Z_ERR_TOO_SMALL);
        }
        if (rule.has_max_len && decoded_len > rule.max_len) {
            res.addIssue(rule.name, Z_ERR_TOO_BIG);
        }

        const slice_len = if (decoded_len < 256) { decoded_len } else { 256 };
        const s: string = decoded[0..slice_len];
        if (rule.kind == Z_EMAIL && !zEmail(s)) {
            res.addIssue(rule.name, Z_ERR_INVALID_EMAIL);
        } else if (rule.kind == Z_URL && !zUrl(s)) {
            res.addIssue(rule.name, Z_ERR_INVALID_URL);
        } else if (rule.kind == Z_UUID && !zUuid(s)) {
            res.addIssue(rule.name, Z_ERR_INVALID_UUID);
        } else if (rule.kind == Z_ENUM && !zEnum(s, rule.enum_csv)) {
            res.addIssue(rule.name, Z_ERR_INVALID_ENUM);
        }
        return;
    }

    if (rule.kind == Z_NUMBER || rule.kind == Z_INT) {
        if (k != KIND_NUMBER) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
            return;
        }
        let val: f64 = 0.0;
        let is_int = false;
        if (!zdParseNumber(raw_val, &val, &is_int)) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
            return;
        }
        if (rule.kind == Z_INT && !is_int) {
            res.addIssue(rule.name, Z_ERR_NOT_INT);
        }
        if (rule.has_min && val < rule.min_val) {
            res.addIssue(rule.name, Z_ERR_TOO_SMALL);
        }
        if (rule.has_max && val > rule.max_val) {
            res.addIssue(rule.name, Z_ERR_TOO_BIG);
        }
        return;
    }

    if (rule.kind == Z_BOOL) {
        if (k != KIND_BOOLEAN) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
        }
        return;
    }

    if (rule.kind == Z_ARRAY) {
        if (k != KIND_ARRAY) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
            return;
        }
        let item_count: usize = 0;
        if (!zdCountJsonArrayItems(raw_val, &item_count)) {
            res.addIssue(rule.name, Z_ERR_INVALID_JSON);
            return;
        }
        if (rule.has_min_len && item_count < rule.min_len) {
            res.addIssue(rule.name, Z_ERR_TOO_SMALL);
        }
        if (rule.has_max_len && item_count > rule.max_len) {
            res.addIssue(rule.name, Z_ERR_TOO_BIG);
        }
        return;
    }

    if (rule.kind == Z_OBJECT) {
        if (k != KIND_OBJECT) {
            res.addIssue(rule.name, Z_ERR_INVALID_TYPE);
        }
        return;
    }
}

/// Scans a JSON value starting at `pos` inside `json[0..end]`, updating `out_end.*`.
fn zdScanJsonValue(json: string, pos: usize, end: usize, out_end: *usize): bool {
    if (pos >= end) {
        return false;
    }
    const c = json[pos];
    if (c == 34) {
        let i = pos + 1;
        while (i < end) {
            if (json[i] == 92) {
                i = i + 2;
                continue;
            }
            if (json[i] == 34) {
                out_end.* = i + 1;
                return true;
            }
            if (json[i] < 32) {
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
            const ch = json[i];
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

    // Primitive literal (number / bool / null)
    let i = pos;
    while (i < end && !zdIsSpace(json[i]) && json[i] != 44 && json[i] != 125 && json[i] != 93) {
        i = i + 1;
    }
    if (i == pos) {
        return false;
    }
    out_end.* = i;
    return true;
}

pub struct ZObjectSchema {
    pub rules: [24]ZFieldRule,
    pub rule_count: usize,

    pub static fn new(): ZObjectSchema {
        let rules: [24]ZFieldRule = undefined;
        let i: usize = 0;
        while (i < Z_MAX_RULES) {
            rules[i] = ZFieldRule.empty();
            i = i + 1;
        }
        return ZObjectSchema {
            rules: rules,
            rule_count: 0,
        };
    }

    pub fn field(*_, rule: ZFieldRule): bool {
        if (_.rule_count >= Z_MAX_RULES) {
            return false;
        }
        _.rules[_.rule_count] = rule;
        _.rule_count = _.rule_count + 1;
        return true;
    }

    /// Parses a top-level JSON object and validates every configured field rule,
    /// collecting all validation issues without short-circuiting.
    pub fn safeParseJson(json: string): ZParseResult {
        let res = ZParseResult.empty();
        let start: usize = 0;
        let end: usize = 0;
        zdTrimBounds(json, &start, &end);

        if (start >= end) {
            res.addIssue("$", Z_ERR_INVALID_JSON);
            return res;
        }

        const root_kind = kindOf(json[start..end]);
        if (root_kind == KIND_INVALID) {
            res.addIssue("$", Z_ERR_INVALID_JSON);
            return res;
        }
        if (root_kind != KIND_OBJECT) {
            res.addIssue("$", Z_ERR_NOT_OBJECT);
            return res;
        }

        let seen: [24]bool = [false; 24];
        let i = start + 1;
        const limit = end - 1;
        while (i < limit && zdIsSpace(json[i])) {
            i = i + 1;
        }

        if (i < limit) {
            while (true) {
                while (i < limit && zdIsSpace(json[i])) {
                    i = i + 1;
                }
                if (i >= limit || json[i] != 34) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }

                // Scan key string
                let key_end: usize = 0;
                if (!zdScanJsonValue(json, i, limit, &key_end)) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }
                let key_buf: [64]u8 = [0; 64];
                let key_len: usize = 0;
                if (!zdDecodeJsonString(json[i..key_end], key_buf[0..64], &key_len)) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }
                const key_slice_len = if (key_len < 64) { key_len } else { 64 };
                i = key_end;

                // Expect ':'
                while (i < limit && zdIsSpace(json[i])) {
                    i = i + 1;
                }
                if (i >= limit || json[i] != 58) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }
                i = i + 1;

                // Scan value
                while (i < limit && zdIsSpace(json[i])) {
                    i = i + 1;
                }
                let val_end: usize = 0;
                if (!zdScanJsonValue(json, i, limit, &val_end)) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }
                const raw_val = json[i..val_end];
                if (kindOf(raw_val) == KIND_INVALID) {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }

                // Match against schema rules
                let r: usize = 0;
                while (r < _.rule_count) {
                    if (zdSliceEqStr(key_buf[0..key_slice_len], _.rules[r].name)) {
                        seen[r] = true;
                        zdValidateFieldValue(_.rules[r], raw_val, &res);
                    }
                    r = r + 1;
                }

                i = val_end;
                while (i < limit && zdIsSpace(json[i])) {
                    i = i + 1;
                }
                if (i >= limit) {
                    break;
                }
                if (json[i] == 44) {
                    i = i + 1;
                    while (i < limit && zdIsSpace(json[i])) {
                        i = i + 1;
                    }
                    if (i >= limit) {
                        res.addIssue("$", Z_ERR_INVALID_JSON);
                        return res;
                    }
                } else {
                    res.addIssue("$", Z_ERR_INVALID_JSON);
                    return res;
                }
            }
        }

        // Check missing required fields
        let r: usize = 0;
        while (r < _.rule_count) {
            if (_.rules[r].required && !seen[r]) {
                res.addIssue(_.rules[r].name, Z_ERR_REQUIRED);
            }
            r = r + 1;
        }

        return res;
    }
}

export {
    Z_STRING,
    Z_NUMBER,
    Z_INT,
    Z_BOOL,
    Z_EMAIL,
    Z_URL,
    Z_UUID,
    Z_ENUM,
    Z_ARRAY,
    Z_OBJECT,
    Z_ERR_INVALID_JSON,
    Z_ERR_NOT_OBJECT,
    Z_ERR_REQUIRED,
    Z_ERR_INVALID_TYPE,
    Z_ERR_NOT_INT,
    Z_ERR_TOO_SMALL,
    Z_ERR_TOO_BIG,
    Z_ERR_INVALID_EMAIL,
    Z_ERR_INVALID_URL,
    Z_ERR_INVALID_UUID,
    Z_ERR_INVALID_ENUM,
    Z_MAX_RULES,
    Z_MAX_ISSUES,
    ZFieldRule,
    ZIssue,
    ZParseResult,
    ZObjectSchema,
    zString,
    zNumber,
    zEmail,
    zUrl,
    zUuid,
    zEnum,
};
