// @cap/nth-check — CSS :nth-child(An+B) parser & matcher in pure CP
// Clean-room implementation of npm `nth-check` (#544, 104M/wk).

import { isSpace, isDigit, trim, eqIgnoreCase } from "cap:ascii";

pub struct NthRule {
    pub a: i32,
    pub b: i32,
    pub valid: bool,

    /// Matches a 0-based element index (matching npm `nth-check` convention:
    /// index 0 corresponds to CSS 1st child, i.e. 1-based position = index + 1).
    pub fn matches(index: usize): bool {
        if (!_.valid) {
            return false;
        }
        const pos: i64 = (index as i64) + 1;
        const a64: i64 = _.a as i64;
        const b64: i64 = _.b as i64;
        if (a64 == 0) {
            return pos == b64;
        }
        const diff = pos - b64;
        if (a64 > 0) {
            if (diff < 0) {
                return false;
            }
            return ((diff as u64) % (a64 as u64)) == 0;
        } else {
            // a < 0: diff must be <= 0 and divisible by -a
            if (diff > 0) {
                return false;
            }
            const neg_diff = (0 - diff) as u64;
            const neg_a = (0 - a64) as u64;
            return (neg_diff % neg_a) == 0;
        }
    }
}

/// Parses a CSS `An+B` formula (`even`, `odd`, `2n+1`, `-n+3`, `5`, `3n-2`, etc.).
pub fn parseNth(formula: string): NthRule {
    const inv = NthRule { a: 0, b: 0, valid: false };
    const s = trim(formula);
    if (s.len == 0) {
        return inv;
    }
    if (eqIgnoreCase(s, "even")) {
        return NthRule { a: 2, b: 0, valid: true };
    }
    if (eqIgnoreCase(s, "odd")) {
        return NthRule { a: 2, b: 1, valid: true };
    }

    // Locate 'n' or 'N'
    let n_pos: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 110 || s[i] == 78) {
            if (n_pos != s.len) {
                return inv; // duplicate 'n'
            }
            n_pos = i;
        }
        i += 1;
    }

    // Case 1: pure integer B (no 'n')
    if (n_pos == s.len) {
        let idx: usize = 0;
        let sign: i32 = 1;
        if (s[0] == 43) {
            idx = 1;
        } else if (s[0] == 45) {
            sign = -1;
            idx = 1;
        }
        if (idx >= s.len) {
            return inv;
        }
        let val: i32 = 0;
        while (idx < s.len) {
            if (!isDigit(s[idx])) {
                return inv;
            }
            val = val * 10 + ((s[idx] - 48) as i32);
            idx += 1;
        }
        return NthRule { a: 0, b: sign * val, valid: true };
    }

    // Case 2: An[±B]
    let a_val: i32 = 1;
    if (n_pos == 0) {
        a_val = 1;
    } else {
        let idx: usize = 0;
        let sign: i32 = 1;
        if (s[0] == 43) {
            idx = 1;
        } else if (s[0] == 45) {
            sign = -1;
            idx = 1;
        }
        if (idx == n_pos) {
            a_val = sign;
        } else {
            let acc: i32 = 0;
            while (idx < n_pos) {
                if (!isDigit(s[idx])) {
                    return inv;
                }
                acc = acc * 10 + ((s[idx] - 48) as i32);
                idx += 1;
            }
            a_val = sign * acc;
        }
    }

    // Parse optional ±B after 'n'
    let pos = n_pos + 1;
    while (pos < s.len && isSpace(s[pos])) {
        pos += 1;
    }
    if (pos == s.len) {
        return NthRule { a: a_val, b: 0, valid: true };
    }

    let b_sign: i32 = 1;
    if (s[pos] == 43) {
        b_sign = 1;
        pos += 1;
    } else if (s[pos] == 45) {
        b_sign = -1;
        pos += 1;
    } else {
        return inv;
    }

    while (pos < s.len && isSpace(s[pos])) {
        pos += 1;
    }
    if (pos >= s.len) {
        return inv;
    }
    let b_acc: i32 = 0;
    while (pos < s.len) {
        if (!isDigit(s[pos])) {
            return inv;
        }
        b_acc = b_acc * 10 + ((s[pos] - 48) as i32);
        pos += 1;
    }
    return NthRule { a: a_val, b: b_sign * b_acc, valid: true };
}

/// Convenience helper: checks if 0-based `index` matches the CSS `formula`.
pub fn nthCheck(formula: string, index: usize): bool {
    const rule = parseNth(formula);
    return rule.matches(index);
}

export {
    NthRule,
    parseNth,
    nthCheck,
};
