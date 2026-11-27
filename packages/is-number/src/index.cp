// @cap/is-number — Zero-heap strict numeric string validator (matching npm is-number 400M+/wk)
// Clean-room MIT implementation in pure CP.

import { isDigit, isHexDigit, trim } from "cap:ascii";

pub fn isNumber(s: string): bool {
    const t = trim(s);
    const end = t.len;
    if (end == 0) { return false; }

    let i: usize = 0;
    // Optional leading sign
    if (t[i] == 43 || t[i] == 45) {
        i = i + 1;
        if (i >= end) { return false; }
    }

    // Hex prefix 0x / 0X (only when no sign, matching JS Number() semantics)
    if (i == 0 && end >= 2 && t[0] == 48 && (t[1] == 120 || t[1] == 88)) {
        let j: usize = 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (!isHexDigit(t[j])) { return false; }
            j = j + 1;
        }
        return true;
    }

    // Binary 0b / 0B or Octal 0o / 0O (unsigned)
    if (i == 0 && end >= 2 && t[0] == 48 && (t[1] == 98 || t[1] == 66)) {
        let j: usize = 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (t[j] != 48 && t[j] != 49) { return false; }
            j = j + 1;
        }
        return true;
    }
    if (i == 0 && end >= 2 && t[0] == 48 && (t[1] == 111 || t[1] == 79)) {
        let j: usize = 2;
        if (j >= end) { return false; }
        while (j < end) {
            if (t[j] < 48 || t[j] > 55) { return false; }
            j = j + 1;
        }
        return true;
    }

    let has_digits = false;
    while (i < end && isDigit(t[i])) {
        has_digits = true;
        i = i + 1;
    }
    if (i < end && t[i] == 46) {
        i = i + 1;
        while (i < end && isDigit(t[i])) {
            has_digits = true;
            i = i + 1;
        }
    }
    if (!has_digits) { return false; }

    if (i < end && (t[i] == 101 || t[i] == 69)) {
        i = i + 1;
        if (i < end && (t[i] == 43 || t[i] == 45)) {
            i = i + 1;
        }
        let has_exp = false;
        while (i < end && isDigit(t[i])) {
            has_exp = true;
            i = i + 1;
        }
        if (!has_exp) { return false; }
    }

    return i == end;
}

pub fn isIntegerStr(s: string): bool {
    const t = trim(s);
    const end = t.len;
    if (end == 0) { return false; }
    let i: usize = 0;
    if (t[i] == 43 || t[i] == 45) {
        i = i + 1;
        if (i >= end) { return false; }
    }
    while (i < end) {
        if (!isDigit(t[i])) { return false; }
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
