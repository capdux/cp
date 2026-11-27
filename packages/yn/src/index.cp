// @cap/yn — Zero-allocation yes/no boolean string parser in pure CP
// Clean-room MIT implementation of npm `yn` (#978, 56.8M/wk).

fn ynToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) { return b + 32; }
    return b;
}

fn ynTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == 32 || s[start] == 9 || s[start] == 10 || s[start] == 13)) {
        start += 1;
    }
    while (end > start && (s[end - 1] == 32 || s[end - 1] == 9 || s[end - 1] == 10 || s[end - 1] == 13)) {
        end -= 1;
    }
    return s[start..end];
}

fn ynEqLower(s: string, target: string): bool {
    if (s.len != target.len) { return false; }
    let i: usize = 0;
    while (i < s.len) {
        if (ynToLower(s[i]) != target[i]) { return false; }
        i += 1;
    }
    return true;
}

/// Parses standard yes/no boolean strings (`y`, `yes`, `true`, `1`, `on` -> `true`;
/// `n`, `no`, `false`, `0`, `off` -> `false`; unrecognized -> `null`).
pub fn yn(input: string): bool? {
    const s = ynTrim(input);
    if (s.len == 0 || s.len > 10) {
        return null;
    }
    if (ynEqLower(s, "y") || ynEqLower(s, "yes") || ynEqLower(s, "true") ||
        ynEqLower(s, "1") || ynEqLower(s, "on")) {
        return true;
    }
    if (ynEqLower(s, "n") || ynEqLower(s, "no") || ynEqLower(s, "false") ||
        ynEqLower(s, "0") || ynEqLower(s, "off")) {
        return false;
    }
    return null;
}

/// Parses yes/no strings with optional `lenient` typo tolerance (e.g. `yse`, `yess`, `noo`, `truu`, `flase`).
pub fn ynLenient(input: string): bool? {
    const strict = yn(input);
    if (strict != null) {
        return strict;
    }
    const s = ynTrim(input);
    if (ynEqLower(s, "yse") || ynEqLower(s, "yess") || ynEqLower(s, "ye") ||
        ynEqLower(s, "ys") || ynEqLower(s, "es") || ynEqLower(s, "truu") || ynEqLower(s, "ture")) {
        return true;
    }
    if (ynEqLower(s, "noo") || ynEqLower(s, "nop") || ynEqLower(s, "nope") ||
        ynEqLower(s, "flase") || ynEqLower(s, "fasle")) {
        return false;
    }
    return null;
}

/// Parses `input` with a `default_val` fallback when unrecognized.
pub fn ynDefault(input: string, default_val: bool): bool {
    return yn(input) ?? default_val;
}

export {
    yn,
    ynLenient,
    ynDefault,
};
