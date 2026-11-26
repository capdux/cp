// @cap/is-number — Zero-heap strict numeric string validator (matching npm is-number 400M+/wk)
// Clean-room MIT implementation in pure CP.

fn isSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13 || c == 11 || c == 12;
}

fn isDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn isHexDigit(c: u8): bool {
    return (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
}

pub fn isNumber(s: string): bool {
    let n = s.len;
    let start: usize = 0;
    while (start < n && isSpace(s[start])) {
        start = start + 1;
    }
    let end: usize = n;
    while (end > start && isSpace(s[end - 1])) {
        end = end - 1;
    }
    if (start >= end) { return false; }

    let i: usize = start;
    // Optional leading sign
    if (s[i] == 43 || s[i] == 45) {
        i = i + 1;
        if (i >= end) { return false; }
    }

    // Hex prefix 0x / 0X (only when no sign, matching JS Number() semantics)
    if (i == start && i + 2 <= end && s[i] == 48 && (s[i + 1] == 120 || s[i + 1] == 88)) {
        let j = i + 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (!isHexDigit(s[j])) { return false; }
            j = j + 1;
        }
        return true;
    }

    // Binary 0b / 0B or Octal 0o / 0O (unsigned)
    if (i == start && i + 2 <= end && s[i] == 48 && (s[i + 1] == 98 || s[i + 1] == 66)) {
        let j = i + 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (s[j] != 48 && s[j] != 49) { return false; }
            j = j + 1;
        }
        return true;
    }
    if (i == start && i + 2 <= end && s[i] == 48 && (s[i + 1] == 111 || s[i + 1] == 79)) {
        let j = i + 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (s[j] < 48 || s[j] > 55) { return false; }
            j = j + 1;
        }
        return true;
    }

    let has_digits = false;
    while (i < end && isDigit(s[i])) {
        has_digits = true;
        i = i + 1;
    }
    if (i < end && s[i] == 46) {
        i = i + 1;
        while (i < end && isDigit(s[i])) {
            has_digits = true;
            i = i + 1;
        }
    }
    if (!has_digits) { return false; }

    if (i < end && (s[i] == 101 || s[i] == 69)) {
        i = i + 1;
        if (i < end && (s[i] == 43 || s[i] == 45)) {
            i = i + 1;
        }
        let has_exp = false;
        while (i < end && isDigit(s[i])) {
            has_exp = true;
            i = i + 1;
        }
        if (!has_exp) { return false; }
    }

    return i == end;
}

pub fn isIntegerStr(s: string): bool {
    let n = s.len;
    let start: usize = 0;
    while (start < n && isSpace(s[start])) { start = start + 1; }
    let end: usize = n;
    while (end > start && isSpace(s[end - 1])) { end = end - 1; }
    if (start >= end) { return false; }
    let i = start;
    if (s[i] == 43 || s[i] == 45) {
        i = i + 1;
        if (i >= end) { return false; }
    }
    while (i < end) {
        if (!isDigit(s[i])) { return false; }
        i = i + 1;
    }
    return true;
}

pub fn isFiniteF64(x: f64): bool {
    // NaN != NaN; Infinity - Infinity is NaN
    if (x != x) { return false; }
    let diff = x - x;
    return diff == 0.0;
}
