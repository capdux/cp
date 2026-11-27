// @cap/has-flag — Check if CLI argv contains a specific flag
// Clean-room MIT CP implementation of npm `has-flag` (250M+/wk)
// Zero-heap, single-pass, honors `--` end-of-options terminator.

fn hfEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn hfStartsWith(s: string, prefix: string): bool {
    if (prefix.len > s.len) { return false; }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn hfMatchesFlag(arg: string, flag: string, prefix_len: usize): bool {
    // Prefix is "-" (len 1) or "--" (len 2)
    let expected_len: usize = prefix_len + flag.len;
    if (arg.len < expected_len) { return false; }
    if (prefix_len == 1) {
        if (arg[0] != 45) { return false; }
    } else if (prefix_len == 2) {
        if (arg[0] != 45 || arg[1] != 45) { return false; }
    }
    let i: usize = 0;
    while (i < flag.len) {
        if (arg[prefix_len + i] != flag[i]) { return false; }
        i = i + 1;
    }
    if (arg.len == expected_len) { return true; }
    // Also match `--flag=value` when flag itself does not already contain '='
    if (arg[expected_len] == 61) {
        let j: usize = 0;
        while (j < flag.len) {
            if (flag[j] == 61) { return false; }
            j = j + 1;
        }
        return true;
    }
    return false;
}

fn hfComputePrefixLen(flag: string): usize {
    if (flag.len == 0) { return 0; }
    if (flag[0] == 45) { return 0; }
    if (flag.len == 1) { return 1; }
    return 2;
}

/// Find the index of `flag` in `argv` before any `--` terminator.
/// Returns `-1` if not found.
/// Follows `has-flag` rules:
/// - Leading `-` or `--` in `flag` is preserved verbatim.
/// - Otherwise single-char flag gets `-` prefix; multi-char flag gets `--` prefix.
/// - Matches both exact `--flag` / `--flag=val` and `--flag=expected` when `=` is in `flag`.
/// - Stops scanning at the `--` end-of-options terminator.
pub fn findFlagIndex(flag: string, argv: []const string): i64 {
    if (flag.len == 0) { return -1; }
    let prefix_len = hfComputePrefixLen(flag);
    let i: usize = 0;
    while (i < argv.len) {
        let arg = argv[i];
        if (hfEq(arg, "--")) {
            return -1;
        }
        if (hfMatchesFlag(arg, flag, prefix_len)) {
            return i as i64;
        }
        i = i + 1;
    }
    return -1;
}

/// Returns `true` if `argv` contains `flag` before any `--` terminator.
pub fn hasFlag(flag: string, argv: []const string): bool {
    return findFlagIndex(flag, argv) >= 0;
}

/// Extracts the value portion of `--flag=value` or `--flag value` from `argv`.
/// Writes value into `out` and returns written length, or `-1` if flag is absent
/// or has no value.
pub fn getFlagValue(flag: string, argv: []const string, out: []u8): i64 {
    let idx = findFlagIndex(flag, argv);
    if (idx < 0) { return -1; }
    let u_idx = idx as usize;
    let arg = argv[u_idx];
    let prefix_len = hfComputePrefixLen(flag);
    let base_len = prefix_len + flag.len;

    // Form 1: --flag=value
    if (arg.len > base_len && arg[base_len] == 61) {
        let val_start = base_len + 1;
        let val_len = arg.len - val_start;
        if (val_len > out.len) { return -1; }
        let k: usize = 0;
        while (k < val_len) {
            out[k] = arg[val_start + k];
            k = k + 1;
        }
        return val_len as i64;
    }

    // Form 2: --flag value (only if next arg exists and does not start with '-')
    if (u_idx + 1 < argv.len) {
        let next_arg = argv[u_idx + 1];
        if (next_arg.len > 0 && next_arg[0] != 45) {
            if (next_arg.len > out.len) { return -1; }
            let k: usize = 0;
            while (k < next_arg.len) {
                out[k] = next_arg[k];
                k = k + 1;
            }
            return next_arg.len as i64;
        }
    }
    return -1;
}

/// Checks if `flag` appears in `argv` AND is not negated by a later `--no-<flag>`.
/// Useful for boolean toggles like `--color` / `--no-color`.
pub fn isFlagEnabled(flag: string, argv: []const string, default_val: bool): bool {
    if (flag.len == 0) { return default_val; }
    let clean = flag;
    if (hfStartsWith(flag, "--")) {
        clean = flag[2..flag.len];
    } else if (hfStartsWith(flag, "-")) {
        clean = flag[1..flag.len];
    }
    if (clean.len == 0) { return default_val; }

    let result = default_val;
    let prefix_len = hfComputePrefixLen(clean);
    let i: usize = 0;
    while (i < argv.len) {
        let arg = argv[i];
        if (hfEq(arg, "--")) { break; }
        if (hfMatchesFlag(arg, clean, prefix_len)) {
            result = true;
        } else if (arg.len == 5 + clean.len && hfStartsWith(arg, "--no-")) {
            let matched = true;
            let j: usize = 0;
            while (j < clean.len) {
                if (arg[5 + j] != clean[j]) {
                    matched = false;
                    break;
                }
                j = j + 1;
            }
            if (matched) {
                result = false;
            }
        }
        i = i + 1;
    }
    return result;
}
