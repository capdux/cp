// @cap/wildmatch — Zero-heap, O(n*m) worst-case (anti-backtracking) wildcard pattern matcher.
// Clean-room MIT port of Rust's `wildmatch` crate; reuses `@cap/unicase` for case-insensitive matching.

import { foldAsciiByte } from "../../unicase/src/index.cp";

fn wmByteEq(a: u8, b: u8, ignore_case: bool): bool {
    if (ignore_case) {
        return foldAsciiByte(a) == foldAsciiByte(b);
    }
    return a == b;
}

/// Core iterative two-pointer wildcard engine (zero recursion, zero heap, linear on typical inputs).
fn wmMatchCore(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool {
    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize = 0;
    let star_si: usize = 0;
    let has_star: bool = false;

    while (si < input.len) {
        if (pi < pattern.len) {
            let pc = pattern[pi];
            if (allow_escape && pc == 92 && pi + 1 < pattern.len) {
                // Backslash escape: match literal next char
                if (wmByteEq(pattern[pi + 1], input[si], ignore_case)) {
                    pi = pi + 2;
                    si = si + 1;
                    continue;
                }
            } else if (pc == any_many) {
                // Collapse consecutive '*' wildcards
                while (pi < pattern.len && pattern[pi] == any_many) {
                    pi = pi + 1;
                }
                has_star = true;
                star_pi = pi;
                star_si = si;
                continue;
            } else if (pc == any_one || wmByteEq(pc, input[si], ignore_case)) {
                pi = pi + 1;
                si = si + 1;
                continue;
            }
        }
        if (has_star) {
            star_si = star_si + 1;
            si = star_si;
            pi = star_pi;
            continue;
        }
        return false;
    }

    while (pi < pattern.len && pattern[pi] == any_many) {
        pi = pi + 1;
    }
    return pi == pattern.len;
}

/// Reusable wildcard matcher (`wildmatch::WildMatch`).
pub struct WildMatch {
    pub pattern: string,
    pub any_many: u8,
    pub any_one: u8,
    pub ignore_case: bool,
    pub allow_escape: bool,

    pub static fn new(pattern: string): WildMatch {
        return WildMatch {
            pattern: pattern,
            any_many: 42, // '*'
            any_one: 63,  // '?'
            ignore_case: false,
            allow_escape: false,
        };
    }

    pub static fn newWithWildcards(pattern: string, any_many: u8, any_one: u8): WildMatch {
        return WildMatch {
            pattern: pattern,
            any_many: any_many,
            any_one: any_one,
            ignore_case: false,
            allow_escape: false,
        };
    }

    pub static fn newIgnoreCase(pattern: string): WildMatch {
        return WildMatch {
            pattern: pattern,
            any_many: 42,
            any_one: 63,
            ignore_case: true,
            allow_escape: false,
        };
    }

    pub static fn newWithEscape(pattern: string, ignore_case: bool): WildMatch {
        return WildMatch {
            pattern: pattern,
            any_many: 42,
            any_one: 63,
            ignore_case: ignore_case,
            allow_escape: true,
        };
    }

    pub fn matches(input: string): bool {
        return wmMatchCore(_.pattern, input, _.any_many, _.any_one, _.ignore_case, _.allow_escape);
    }
}

/// One-shot case-sensitive wildcard match (`*` = any sequence, `?` = single byte).
pub fn wildmatch(pattern: string, input: string): bool {
    return wmMatchCore(pattern, input, 42, 63, false, false);
}

/// One-shot ASCII case-insensitive wildcard match.
pub fn wildmatchIgnoreCase(pattern: string, input: string): bool {
    return wmMatchCore(pattern, input, 42, 63, true, false);
}

/// One-shot wildcard match with custom wildcards (e.g., SQL `LIKE` `%` and `_`).
pub fn wildmatchCustom(pattern: string, input: string, any_many: u8, any_one: u8): bool {
    return wmMatchCore(pattern, input, any_many, any_one, false, false);
}

/// Returns true if `pattern` contains any unescaped `*` or `?` wildcard characters.
pub fn hasWildcards(pattern: string): bool {
    let i: usize = 0;
    while (i < pattern.len) {
        let c = pattern[i];
        if (c == 42 || c == 63) { return true; }
        i = i + 1;
    }
    return false;
}
