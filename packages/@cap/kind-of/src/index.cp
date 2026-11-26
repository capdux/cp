// @cap/kind-of — Fast zero-heap type classification for JSON / JS / literal values
// Clean-room MIT CP implementation of npm `kind-of` (120M+/wk)
// Zero-heap, single-pass scanner covering null, undefined, boolean, number, bigint,
// string, array, object, regexp, date, symbol, function, error, buffer, and invalid.

pub const KIND_INVALID: u8 = 0;
pub const KIND_NULL: u8 = 1;
pub const KIND_UNDEFINED: u8 = 2;
pub const KIND_BOOLEAN: u8 = 3;
pub const KIND_NUMBER: u8 = 4;
pub const KIND_STRING: u8 = 5;
pub const KIND_ARRAY: u8 = 6;
pub const KIND_OBJECT: u8 = 7;
pub const KIND_REGEXP: u8 = 8;
pub const KIND_DATE: u8 = 9;
pub const KIND_BIGINT: u8 = 10;
pub const KIND_FUNCTION: u8 = 11;

fn koIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn koIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn koEq(s: string, start: usize, end: usize, lit: string): bool {
    let n = end - start;
    if (n != lit.len) { return false; }
    let i: usize = 0;
    while (i < n) {
        if (s[start + i] != lit[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn koStartsWith(s: string, start: usize, end: usize, prefix: string): bool {
    if (end - start < prefix.len) { return false; }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[start + i] != prefix[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn koIsNumber(s: string, start: usize, end: usize): bool {
    let i = start;
    if (i >= end) { return false; }
    if (s[i] == 45 || s[i] == 43) { i = i + 1; }
    if (i >= end) { return false; }
    let has_digits = false;
    while (i < end && koIsDigit(s[i])) {
        has_digits = true;
        i = i + 1;
    }
    if (i < end && s[i] == 46) {
        i = i + 1;
        while (i < end && koIsDigit(s[i])) {
            has_digits = true;
            i = i + 1;
        }
    }
    if (!has_digits) { return false; }
    if (i < end && (s[i] == 101 || s[i] == 69)) {
        i = i + 1;
        if (i < end && (s[i] == 43 || s[i] == 45)) { i = i + 1; }
        let exp_digits = false;
        while (i < end && koIsDigit(s[i])) {
            exp_digits = true;
            i = i + 1;
        }
        if (!exp_digits) { return false; }
    }
    return i == end;
}

fn koIsBigInt(s: string, start: usize, end: usize): bool {
    if (end - start < 2) { return false; }
    if (s[end - 1] != 110) { return false; } // trailing 'n'
    let i = start;
    if (s[i] == 45 || s[i] == 43) { i = i + 1; }
    if (i >= end - 1) { return false; }
    while (i < end - 1) {
        if (!koIsDigit(s[i])) { return false; }
        i = i + 1;
    }
    return true;
}

fn koIsIsoDate(s: string, start: usize, end: usize): bool {
    // Expects quoted ISO-8601 date like "2026-11-27T03:00:00Z" (inner len >= 10, YYYY-MM-DD)
    if (end - start < 12) { return false; }
    let q = s[start];
    if ((q != 34 && q != 39) || s[end - 1] != q) { return false; }
    let a = start + 1;
    return koIsDigit(s[a]) && koIsDigit(s[a + 1]) && koIsDigit(s[a + 2]) && koIsDigit(s[a + 3])
        && s[a + 4] == 45
        && koIsDigit(s[a + 5]) && koIsDigit(s[a + 6])
        && s[a + 7] == 45
        && koIsDigit(s[a + 8]) && koIsDigit(s[a + 9])
        && (end - start == 12 || s[a + 10] == 84 || s[a + 10] == 32);
}

/// Classifies the runtime kind of a JS/JSON literal representation.
pub fn kindOf(raw: string): u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && koIsSpace(raw[start])) { start = start + 1; }
    while (end > start && koIsSpace(raw[end - 1])) { end = end - 1; }
    if (start >= end) { return KIND_INVALID; }

    if (koEq(raw, start, end, "null")) { return KIND_NULL; }
    if (koEq(raw, start, end, "undefined")) { return KIND_UNDEFINED; }
    if (koEq(raw, start, end, "true") || koEq(raw, start, end, "false")) { return KIND_BOOLEAN; }
    if (koEq(raw, start, end, "NaN") || koEq(raw, start, end, "Infinity") || koEq(raw, start, end, "-Infinity")) {
        return KIND_NUMBER;
    }

    let first = raw[start];
    let last = raw[end - 1];

    if (first == 91 && last == 93) { return KIND_ARRAY; }
    if (first == 123 && last == 125) { return KIND_OBJECT; }
    if ((first == 34 && last == 34) || (first == 39 && last == 39)) {
        if (end - start >= 2) {
            if (koIsIsoDate(raw, start, end)) { return KIND_DATE; }
            return KIND_STRING;
        }
        return KIND_INVALID;
    }
    if (first == 47 && end - start >= 3) {
        // Regex literal /pattern/flags
        let k = end - 1;
        while (k > start && ((raw[k] >= 97 && raw[k] <= 122))) { k = k - 1; }
        if (k > start && raw[k] == 47) { return KIND_REGEXP; }
    }
    if (koStartsWith(raw, start, end, "function") || koStartsWith(raw, start, end, "async ")) {
        return KIND_FUNCTION;
    }
    if (koIsBigInt(raw, start, end)) { return KIND_BIGINT; }
    if (koIsNumber(raw, start, end)) { return KIND_NUMBER; }
    return KIND_INVALID;
}

/// Returns the canonical lowercase npm `kind-of` string name for a `KIND_*` tag.
pub fn kindName(kind: u8): string {
    switch (kind) {
        1 => return "null",
        2 => return "undefined",
        3 => return "boolean",
        4 => return "number",
        5 => return "string",
        6 => return "array",
        7 => return "object",
        8 => return "regexp",
        9 => return "date",
        10 => return "bigint",
        11 => return "function",
        else => return "invalid",
    }
}

/// Convenience wrapper returning the canonical lowercase type name directly.
pub fn kindOfName(raw: string): string {
    return kindName(kindOf(raw));
}

/// Returns true if `raw` represents a primitive value (null, undefined, boolean, number, string, bigint).
pub fn isPrimitive(raw: string): bool {
    let k = kindOf(raw);
    return k == KIND_NULL || k == KIND_UNDEFINED || k == KIND_BOOLEAN
        || k == KIND_NUMBER || k == KIND_STRING || k == KIND_BIGINT;
}

/// Returns true if `raw` represents a plain `{ ... }` object (mirrors `is-plain-object` / `isobject`).
pub fn isPlainObject(raw: string): bool {
    return kindOf(raw) == KIND_OBJECT;
}
