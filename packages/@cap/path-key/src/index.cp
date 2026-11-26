// @cap/path-key — Zero-heap cross-platform PATH env key resolver (matching npm path-key 250M+/wk)
// Clean-room MIT implementation in pure CP.

pub struct PathSpan {
    pub start: usize,
    pub len: usize,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 65 && c <= 90) { return c + 32; }
    return c;
}

pub fn isPathKeyName(key: string): bool {
    if (key.len != 4) { return false; }
    return toLowerAscii(key[0]) == 112
        && toLowerAscii(key[1]) == 97
        && toLowerAscii(key[2]) == 116
        && toLowerAscii(key[3]) == 104;
}

// Given a list of environment variable names and whether platform is Windows,
// returns the index of the effective PATH key (last case-insensitive match on Windows,
// exact "PATH" on POSIX), or -1 if none found.
pub fn findPathKeyIndex(keys: []string, is_windows: bool): i64 {
    if (!is_windows) {
        let i: usize = 0;
        while (i < keys.len) {
            let k = keys[i];
            if (k.len == 4 && k[0] == 80 && k[1] == 65 && k[2] == 84 && k[3] == 72) {
                return i as i64;
            }
            i = i + 1;
        }
        return -1;
    }
    let found: i64 = -1;
    let i: usize = 0;
    while (i < keys.len) {
        if (isPathKeyName(keys[i])) {
            found = i as i64;
        }
        i = i + 1;
    }
    return found;
}

pub fn defaultPathKey(is_windows: bool): string {
    if (is_windows) { return "Path"; }
    return "PATH";
}

pub fn pathDelimiter(is_windows: bool): u8 {
    if (is_windows) { return 59; } // ';'
    return 58; // ':'
}

// Split a PATH string into non-empty entry spans using platform delimiter (':' or ';').
pub fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize {
    let delim = pathDelimiter(is_windows);
    let n = path_val.len;
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= n) {
        let at_sep = (i == n) || (path_val[i] == delim);
        if (at_sep) {
            let seg_len = i - start;
            if (seg_len > 0 && count < out.len) {
                out[count] = PathSpan { start: start, len: seg_len };
                count = count + 1;
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return count;
}
