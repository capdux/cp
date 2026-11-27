// @cap/is-glob — Zero-allocation glob pattern detector (reuses @cap/is-extglob)
// MIT License

import { isExtglob } from "../../is-extglob/src/index.cp";

fn hasClosedBracket(str: string, open_idx: usize): bool {
    let j: usize = open_idx + 1;
    let count: usize = 0;
    while (j < str.len) {
        let ch: u8 = str[j];
        if (ch == 92) {
            if (j + 1 < str.len) {
                j = j + 2;
                count = count + 1;
            } else {
                j = j + 1;
            }
        } else if (ch == 93) {
            // Require non-empty character class: [...]
            return count > 0;
        } else {
            count = count + 1;
            j = j + 1;
        }
    }
    return false;
}

fn hasClosedBrace(str: string, open_idx: usize): bool {
    let j: usize = open_idx + 1;
    let depth: usize = 1;
    let count: usize = 0;
    while (j < str.len) {
        let ch: u8 = str[j];
        if (ch == 92) {
            if (j + 1 < str.len) {
                j = j + 2;
                count = count + 1;
            } else {
                j = j + 1;
            }
        } else if (ch == 123) {
            depth = depth + 1;
            count = count + 1;
            j = j + 1;
        } else if (ch == 125) {
            depth = depth - 1;
            if (depth == 0) {
                return count > 0;
            }
            count = count + 1;
            j = j + 1;
        } else {
            count = count + 1;
            j = j + 1;
        }
    }
    return false;
}

pub fn hasUnescapedWildcard(str: string): bool {
    let i: usize = 0;
    while (i < str.len) {
        let ch: u8 = str[i];
        if (ch == 92) {
            if (i + 1 < str.len) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (ch == 42 || ch == 63) {
            // '*' (42) or '?' (63)
            return true;
        } else {
            i = i + 1;
        }
    }
    return false;
}

pub fn isGlob(str: string, strict: bool): bool {
    if (str.len == 0) {
        return false;
    }

    // Reuse @cap/is-extglob for @(...), !(...), *(...), +(...), ?(...)
    if (isExtglob(str)) {
        return true;
    }

    // Leading negation '!'
    if (str[0] == 33) {
        return true;
    }

    let i: usize = 0;
    while (i < str.len) {
        let ch: u8 = str[i];
        if (ch == 92) {
            // Skip escaped byte
            if (i + 1 < str.len) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (ch == 42 || ch == 63) {
            // '*' or '?'
            return true;
        } else if (ch == 91) {
            // '['
            if (!strict || hasClosedBracket(str, i)) {
                return true;
            }
            i = i + 1;
        } else if (ch == 123) {
            // '{'
            if (!strict || hasClosedBrace(str, i)) {
                return true;
            }
            i = i + 1;
        } else if (!strict && (ch == 40 || ch == 41 || ch == 93 || ch == 125)) {
            // In non-strict mode, unmatched grouping/bracket chars count as glob tokens
            return true;
        } else {
            i = i + 1;
        }
    }
    return false;
}

pub fn isGlobDefault(str: string): bool {
    return isGlob(str, true);
}
