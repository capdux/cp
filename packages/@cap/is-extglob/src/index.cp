// @cap/is-extglob — Zero-allocation Bash/Micromatch extglob pattern detector
// MIT License

fn isExtglobPrefix(ch: u8): bool {
    // '@' (64), '!' (33), '*' (42), '+' (43), '?' (63)
    return ch == 64 || ch == 33 || ch == 42 || ch == 43 || ch == 63;
}

// Given str[start] is an extglob prefix and str[start + 1] == '(',
// returns the byte index right after the matching unescaped ')', or 0 if unclosed.
fn matchExtglobEnd(str: string, start: usize): usize {
    let depth: usize = 1;
    let j: usize = start + 2;
    while (j < str.len) {
        let ch: u8 = str[j];
        if (ch == 92) {
            // Backslash escapes the next byte
            if (j + 1 < str.len) {
                j = j + 2;
            } else {
                j = j + 1;
            }
        } else if (ch == 40) {
            depth = depth + 1;
            j = j + 1;
        } else if (ch == 41) {
            depth = depth - 1;
            j = j + 1;
            if (depth == 0) {
                return j;
            }
        } else {
            j = j + 1;
        }
    }
    return 0;
}

pub fn findExtglobStart(str: string): i64 {
    if (str.len < 3) {
        return -1;
    }
    let i: usize = 0;
    while (i < str.len) {
        let ch: u8 = str[i];
        if (ch == 92) {
            if (i + 1 < str.len) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (isExtglobPrefix(ch) && i + 1 < str.len && str[i + 1] == 40) {
            let end_pos: usize = matchExtglobEnd(str, i);
            if (end_pos > 0) {
                return i as i64;
            }
            i = i + 1;
        } else {
            i = i + 1;
        }
    }
    return -1;
}

pub fn isExtglob(str: string): bool {
    return findExtglobStart(str) >= 0;
}

pub fn countExtglobs(str: string): usize {
    if (str.len < 3) {
        return 0;
    }
    let count: usize = 0;
    let i: usize = 0;
    while (i < str.len) {
        let ch: u8 = str[i];
        if (ch == 92) {
            if (i + 1 < str.len) {
                i = i + 2;
            } else {
                i = i + 1;
            }
        } else if (isExtglobPrefix(ch) && i + 1 < str.len && str[i + 1] == 40) {
            let end_pos: usize = matchExtglobEnd(str, i);
            if (end_pos > 0) {
                count = count + 1;
                i = end_pos;
            } else {
                i = i + 1;
            }
        } else {
            i = i + 1;
        }
    }
    return count;
}
