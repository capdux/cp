// @cap/ccount — Zero-allocation character/substring counter & longest streak finder in pure CP
// Clean-room MIT implementation of npm `ccount` (#617, 92.9M/wk) & `longest-streak` (#671, 86.0M/wk).

/// Counts non-overlapping occurrences of single byte `ch` in `value`.
pub fn ccountByte(value: string, ch: u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < value.len) {
        if (value[i] == ch) {
            count += 1;
        }
        i += 1;
    }
    return count;
}

/// Counts non-overlapping occurrences of `needle` inside `value`.
pub fn ccount(value: string, needle: string): usize {
    if (needle.len == 0 || needle.len > value.len) {
        return 0;
    }
    if (needle.len == 1) {
        return ccountByte(value, needle[0]);
    }
    let count: usize = 0;
    let i: usize = 0;
    const limit = value.len - needle.len;
    while (i <= limit) {
        let matched = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (value[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            count += 1;
            i += needle.len;
        } else {
            i += 1;
        }
    }
    return count;
}

/// Returns the length of the longest consecutive run of byte `ch` in `value`.
pub fn longestStreakByte(value: string, ch: u8): usize {
    let best: usize = 0;
    let cur: usize = 0;
    let i: usize = 0;
    while (i < value.len) {
        if (value[i] == ch) {
            cur += 1;
            if (cur > best) {
                best = cur;
            }
        } else {
            cur = 0;
        }
        i += 1;
    }
    return best;
}

/// Returns the length of the longest consecutive run of `needle[0]` (or `needle` repeats) in `value`.
pub fn longestStreak(value: string, needle: string): usize {
    if (needle.len == 0 || needle.len > value.len) {
        return 0;
    }
    if (needle.len == 1) {
        return longestStreakByte(value, needle[0]);
    }
    let best: usize = 0;
    let i: usize = 0;
    while (i + needle.len <= value.len) {
        let runs: usize = 0;
        let pos = i;
        while (pos + needle.len <= value.len) {
            let matched = true;
            let j: usize = 0;
            while (j < needle.len) {
                if (value[pos + j] != needle[j]) {
                    matched = false;
                    break;
                }
                j += 1;
            }
            if (!matched) {
                break;
            }
            runs += 1;
            pos += needle.len;
        }
        if (runs > best) {
            best = runs;
        }
        if (runs > 0) {
            i = pos;
        } else {
            i += 1;
        }
    }
    return best;
}

export {
    ccountByte,
    ccount,
    longestStreakByte,
    longestStreak,
};
